#include "p2p_node.h"
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>
#include "wallet.h"

using json = nlohmann::json;

P2PNode::P2PNode(const std::string& host, int port, std::shared_ptr<Blockchain> blockchain)
    : host_(host), port_(port), blockchain_(blockchain),
      acceptor_(io_context_, tcp::endpoint(boost::asio::ip::make_address(host), port)),
      running_(false) {
}

P2PNode::~P2PNode() {
    stop();
}

void P2PNode::start() {

    std::cout << "  " << host_ << ":" << port_ << " Starting node..." << std::endl;

    running_ = true;
    message_thread_ = std::thread(&P2PNode::messageLoop, this);
    std::cout << "  " << host_ << ":" << port_ << " messageLoop..." << std::endl;
    // 在单独的线程中运行 io_context_
    io_thread_ = std::thread([this]() {
        try {
            handleNewConnection();
            io_context_.run();
        } catch (const std::exception& e) {
            std::cerr << "IO thread error: " << e.what() << std::endl;
        }
    });
    std::cout << "  " << host_ << ":" << port_ << " Node started" << std::endl;
}

void P2PNode::stop() {
    running_ = false;
    
    // 1. 先关闭所有连接
    for (auto& conn : connections_) {
        try {
            conn.second->close();
        } catch (...) {
            // 忽略关闭时的错误
        }
    }
    connections_.clear();
    
    // 2. 然后停止 io_context_
    io_context_.stop();
    
    // 3. 最后等待线程结束
    if (io_thread_.joinable()) {
        io_thread_.join();
    }

    queue_cv_.notify_one();
    if (message_thread_.joinable()) {
        message_thread_.join();
    }
}

void P2PNode::connect(const std::string& host, int port) {
    try {
        // 1. 创建socket
        auto socket = std::make_shared<tcp::socket>(io_context_);
        // 2. 连接到目标节点
        socket->connect(tcp::endpoint(boost::asio::ip::make_address(host), port));
        // 3. 保存连接信息
        std::string nodeId = host + ":" + std::to_string(port);
        connections_[nodeId] = socket;
        // 4. 发送握手消息
        Message handshake;
        handshake.type = MessageType::HANDSHAKE;
        handshake.sender = host_ + ":" + std::to_string(port_);
        sendToNode(nodeId, handshake);
        // 5. 打印连接信息 所以这里不是打印连接信息，而是发送握手消息
        std::cout << "Connected to node: " << nodeId << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to connect to node: " << e.what() << std::endl;
    }
}

void P2PNode::broadcast(const Message& message) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    for (const auto& [nodeId, socket] : connections_) {
        sendToNode(nodeId, message);
    }
}

void P2PNode::sendToNode(const std::string& nodeId, const Message& message) {
    try {
        auto it = connections_.find(nodeId);
        if (it != connections_.end()) {
            std::string serialized = serializeMessage(message);
            boost::asio::write(*it->second, boost::asio::buffer(serialized));
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to send message to node " << nodeId << ": " << e.what() << std::endl;
        connections_.erase(nodeId);
    }
}

std::vector<std::string> P2PNode::getConnectedNodes() const {
    std::vector<std::string> nodes;
    for (const auto& [nodeId, _] : connections_) {
        nodes.push_back(nodeId);
    }
    return nodes;
}

void P2PNode::handleNewConnection() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket) {
            // 1. 获取连接信息 
            if (!ec) {
                std::string nodeId = socket.remote_endpoint().address().to_string() + ":" + 
                                   std::to_string(socket.remote_endpoint().port());
                std::cout << "  " << host_ << ":" << port_ << " New connection from: " << nodeId << std::endl;
                // 2. 保存连接信息
                connections_[nodeId] = std::make_shared<tcp::socket>(std::move(socket));
                // 3. 开始异步读取 所以这里不是开始异步读取，而是设置异步读取
                auto buffer = std::make_shared<boost::asio::streambuf>();
                boost::asio::async_read_until(*connections_[nodeId], *buffer, "\n",
                    [this, nodeId, buffer](boost::system::error_code ec, std::size_t length) {
                        // 4. 处理读取到的数据
                        if (!ec) {
                            // 5. 将数据转换为消息
                            std::string data(boost::asio::buffers_begin(buffer->data()),
                                          boost::asio::buffers_begin(buffer->data()) + length);
                            buffer->consume(length);
                            Message message = deserializeMessage(data);
                            // 6. 处理消息
                            // handleMessage(message, nodeId);
                            // 将消息放入队列
                            {
                                std::unique_lock<std::mutex> lock(queue_mutex_);
                                message_queue_.push(message);
                            }
                            // 通知条件变量
                            queue_cv_.notify_one();
                        }
                    });
            }
            handleNewConnection();
        });
}

void P2PNode::handleMessage(const Message& message, const std::string& sender) {
    switch (message.type) {
        case MessageType::HANDSHAKE:
            std::cout << "  " << host_ << ":" << port_ << " Received handshake from: " << sender << std::endl;
            break;
            
        case MessageType::NEW_BLOCK: {
            // 处理新区块
            std::cout << "  " << host_ << ":" << port_ << " Received new block from: " << sender << std::endl;
            json blockData = json::parse(message.data);
            Block newBlock(blockData);
            blockchain_->addBlock(newBlock.getTransactions());
            // 广播给其他节点
            broadcast(message);
            break;
        }
        
        case MessageType::NEW_TRANSACTION: {
            // 处理新交易
            std::cout << "  " << host_ << ":" << port_ << " Received new transaction from: " << sender << std::endl;
            json txData = json::parse(message.data);
            Transaction newTx(txData);
            if (blockchain_->addTransactionToPool(newTx)) {
                // 广播给其他节点
                broadcast(message);
            }
            break;
        }
        
        case MessageType::GET_BLOCKS: {
            // 发送区块数据
            std::cout << "  " << host_ << ":" << port_ << " Received get blocks from: " << sender << std::endl;
            json request = json::parse(message.data);
            int startHeight = request["start_height"];
            std::vector<Block> blocks = blockchain_->getBlocksFromHeight(startHeight);
            
            Message response;
            response.type = MessageType::BLOCKS;
            
            // 将区块列表转换为 JSON 数组
            json blocksArray = json::array();
            for (const auto& block : blocks) {
                json blockJson = json::parse(block.toJson());
                blocksArray.push_back(blockJson);
            }
            response.data = blocksArray.dump();
            
            sendToNode(sender, response);
            break;
        }
        
        case MessageType::BLOCKS: {
            // 处理接收到的区块
            std::cout << "  " << host_ << ":" << port_ << " Received blocks from: " << sender << std::endl;
            json blocksData = json::parse(message.data);
            for (const auto& blockData : blocksData) {
                Block block(blockData);
                blockchain_->addBlock(block.getTransactions());
            }
            break;
        }
        
        case MessageType::GET_PEERS: {
            // 发送已知节点列表
            std::cout << "  " << host_ << ":" << port_ << " Sending peers to: " << sender << std::endl;
            Message response;
            response.type = MessageType::PEERS;
            response.data = json(getConnectedNodes()).dump();
            sendToNode(sender, response);
            break;
        }
        
        case MessageType::PEERS: {
            // 处理接收到的节点列表
            std::cout << "  " << host_ << ":" << port_ << " Received peers from: " << sender << std::endl;
            json peers = json::parse(message.data);
            for (const auto& peer : peers) {
                std::string host = peer["host"];
                int port = peer["port"];
                connect(host, port);
            }
            break;
        }
        
        case MessageType::GET_UTXOS: {
            handleUTXOsRequest(message, sender);
            break;
        }
        
        case MessageType::UTXOS: {
            json utxosData = json::parse(message.data);
            // 处理接收到的UTXO数据
            break;
        }
        
        case MessageType::GET_BALANCE: {
            handleBalanceRequest(message, sender);
            break;
        }
        
        case MessageType::BALANCE: {
            json balanceData = json::parse(message.data);
            // 处理接收到的余额数据
            break;
        }
        
        case MessageType::SYNC_REQUEST: {
            handleSyncRequest(message, sender);
            break;
        }
        
        case MessageType::SYNC_RESPONSE: {
            json syncData = json::parse(message.data);
            // 处理同步响应数据
            break;
        }
        
        case MessageType::MINING_REQUEST: {
            handleMiningRequest(message, sender);
            break;
        }
        
        case MessageType::MINING_RESPONSE: {
            json miningData = json::parse(message.data);
            // 处理挖矿响应数据
            break;
        }
        
        case MessageType::CONSENSUS_VOTE: {
            handleConsensusVote(message, sender);
            break;
        }
        
        case MessageType::CONSENSUS_RESULT: {
            handleConsensusResult(message, sender);
            break;
        }
    }
}

void P2PNode::messageLoop() {
    std::cout << "  " << host_ << ":" << port_ << " Starting message loop..." << std::endl;

    while (running_) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        queue_cv_.wait(lock, [this] { 
            return !message_queue_.empty() || !running_; 
        });
        
        if (!running_) break;
        
        Message message = message_queue_.front();
        message_queue_.pop();
        lock.unlock();
        
        handleMessage(message, message.sender);
    }
}

bool P2PNode::verifyMessage(const Message& message) {
    // TODO: 实现消息验证逻辑
    std::cout << "  " << host_ << ":" << port_ << " Verifying message..." << std::endl;
    auto result = Wallet::verify(message.data, message.signature, message.sender);
    std::cout << "  " << host_ << ":" << port_ << " Message verification result: " << result << std::endl;
    return result;
}

std::string P2PNode::serializeMessage(const Message& message) {
    json j;
    j["type"] = static_cast<int>(message.type);
    j["data"] = message.data;
    j["sender"] = message.sender;
    j["signature"] = message.signature;
    return j.dump() + "\n";
}

Message P2PNode::deserializeMessage(const std::string& data) {
    json j = json::parse(data);
    Message message;
    message.type = static_cast<MessageType>(j["type"]);
    message.data = j["data"];
    message.sender = j["sender"];
    message.signature = j["signature"];
    return message;
}

void P2PNode::requestUTXOs(const std::string& address) {
    Message msg;
    msg.type = MessageType::GET_UTXOS;
    msg.data = json({{"address", address}}).dump();
    broadcast(msg);
}

void P2PNode::requestBalance(const std::string& address) {
    Message msg;
    msg.type = MessageType::GET_BALANCE;
    msg.data = json({{"address", address}}).dump();
    broadcast(msg);
}

void P2PNode::requestSync(int startHeight) {
    Message msg;
    msg.type = MessageType::SYNC_REQUEST;
    msg.data = json({{"start_height", startHeight}}).dump();
    broadcast(msg);
}

void P2PNode::requestMining(const std::vector<Transaction>& transactions) {
    Message msg;
    msg.type = MessageType::MINING_REQUEST;
    json txArray = json::array();
    for (const auto& tx : transactions) {
        txArray.push_back(json::parse(tx.toJson()));
    }
    msg.data = txArray.dump();
    broadcast(msg);
}

void P2PNode::broadcastConsensusVote(const std::string& blockHash, bool vote) {
    Message msg;
    msg.type = MessageType::CONSENSUS_VOTE;
    msg.data = json({
        {"block_hash", blockHash},
        {"vote", vote}
    }).dump();
    broadcast(msg);
}

void P2PNode::broadcastConsensusResult(const std::string& blockHash, bool accepted) {
    Message msg;
    msg.type = MessageType::CONSENSUS_RESULT;
    msg.data = json({
        {"block_hash", blockHash},
        {"accepted", accepted}
    }).dump();
    broadcast(msg);
}

void P2PNode::handleUTXOsRequest(const Message& message, const std::string& sender) {
    json request = json::parse(message.data);
    std::string address = request["address"];
    
    // 获取UTXO数据
    auto utxos = blockchain_->getUTXOsForAddress(address);
    
    // 构建响应
    Message response;
    response.type = MessageType::UTXOS;
    json utxosArray = json::array();
    for (const auto& utxo : utxos) {
        utxosArray.push_back(json::parse(utxo.toJson()));
    }
    response.data = utxosArray.dump();
    
    sendToNode(sender, response);
}

void P2PNode::handleBalanceRequest(const Message& message, const std::string& sender) {
    json request = json::parse(message.data);
    std::string address = request["address"];
    
    // 获取余额
    double balance = blockchain_->getBalance(address);
    
    // 构建响应
    Message response;
    response.type = MessageType::BALANCE;
    response.data = json({{"address", address}, {"balance", balance}}).dump();
    
    sendToNode(sender, response);
}

void P2PNode::handleSyncRequest(const Message& message, const std::string& sender) {
    json request = json::parse(message.data);
    int startHeight = request["start_height"];
    
    // 获取区块数据
    auto blocks = blockchain_->getBlocksFromHeight(startHeight);
    
    // 构建响应
    Message response;
    response.type = MessageType::SYNC_RESPONSE;
    json blocksArray = json::array();
    for (const auto& block : blocks) {
        blocksArray.push_back(json::parse(block.toJson()));
    }
    response.data = blocksArray.dump();
    
    sendToNode(sender, response);
}

void P2PNode::handleMiningRequest(const Message& message, const std::string& sender) {
    json request = json::parse(message.data);
    std::vector<Transaction> transactions;
    
    // 解析交易数据
    for (const auto& txData : request) {
        transactions.push_back(Transaction(txData));
    }
    
    // 创建新区块
    blockchain_->addBlock(transactions);
    
    // 构建响应
    Message response;
    response.type = MessageType::MINING_RESPONSE;
    response.data = blockchain_->getChain().back()->toJson();
    
    sendToNode(sender, response);
}

void P2PNode::handleConsensusVote(const Message& message, const std::string& sender) {
    json voteData = json::parse(message.data);
    std::string blockHash = voteData["block_hash"];
    bool vote = voteData["vote"];
    
    // 处理共识投票
    // TODO: 实现共识机制
}

void P2PNode::handleConsensusResult(const Message& message, const std::string& sender) {
    json resultData = json::parse(message.data);
    std::string blockHash = resultData["block_hash"];
    bool accepted = resultData["accepted"];
    
    // 处理共识结果
    // TODO: 实现共识机制
}

void P2PNode::startIPC() {
    // 创建命名管道
    std::string pipe_name = "\\\\.\\pipe\\blockchain_node_" + host_ + "_" + std::to_string(port_);
    std::cout << "Creating named pipe: " << pipe_name << std::endl;
    pipe_handle_ = CreateNamedPipeA(
        pipe_name.c_str(),
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,  // 最大实例数
        0,  // 输出缓冲区大小
        0,  // 输入缓冲区大小
        0,  // 默认超时
        NULL  // 安全属性
    );
    
    if (pipe_handle_ == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Failed to create named pipe");
    }
    
    // 启动IPC线程
    ipc_thread_ = std::thread(&P2PNode::ipcLoop, this);
}

void P2PNode::stopIPC() {
    std::cout << "Stopping IPC" << std::endl;
    if (pipe_handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(pipe_handle_);
        pipe_handle_ = INVALID_HANDLE_VALUE;
    }
    std::cout << "IPC stopped" << std::endl;
    if (ipc_thread_.joinable()) {
        std::cout << "Joining IPC thread" << std::endl;
        ipc_thread_.join();
        std::cout << "IPC thread joined" << std::endl;
    }
    std::cout << "IPC stopped" << std::endl;
}

void P2PNode::ipcLoop() {
    std::cout << "Starting IPC loop" << std::endl;
    while (running_) {
        // 等待客户端连接
        if (!ConnectNamedPipe(pipe_handle_, NULL)) {
            if (GetLastError() != ERROR_PIPE_CONNECTED) {
                continue;
            }
        }
        
        // 读取消息
        char buffer[256];
        DWORD bytes_read;
        if (ReadFile(pipe_handle_, buffer, sizeof(buffer) - 1, &bytes_read, NULL)) {
            buffer[bytes_read] = '\0';
            std::string message(buffer);
            std::cout << "Received message: " << message << std::endl;
            if (message == "EXIT") {
                std::cout << "Received exit signal" << std::endl;
                exit_requested_ = true;
                break;
            }
        }
        
        // 断开连接
        DisconnectNamedPipe(pipe_handle_);
    }
    std::cout << "IPC loop ended" << std::endl;
} 

bool P2PNode::isExitRequested() const {
    std::cout << "Checking exit requested: " << exit_requested_ << std::endl;
    return exit_requested_;
}
