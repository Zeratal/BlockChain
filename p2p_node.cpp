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
        // 检查是否已经连接
        std::string nodeId = host + ":" + std::to_string(port);
        if (connections_.find(nodeId) != connections_.end()) {
            std::cout << "Already connected to node: " << nodeId << std::endl;
            return;
        }
        
        // 检查是否是本地节点
        if (host == host_ && port == port_) {
            std::cout << "Cannot connect to self" << std::endl;
            return;
        }
        
        // 创建socket
        auto socket = std::make_shared<tcp::socket>(io_context_);
        
        // 连接到目标节点
        socket->connect(tcp::endpoint(boost::asio::ip::make_address(host), port));
        
        // 保存连接信息
        connections_[nodeId] = socket;
        
        // 发送握手消息
        Message handshake;
        handshake.type = MessageType::HANDSHAKE;
        handshake.sender = host_ + ":" + std::to_string(port_);
        sendToNode(nodeId, handshake);
        
        std::cout << "Connected to node: " << nodeId << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to connect to node: " << e.what() << std::endl;
    }
}

void P2PNode::broadcast(const Message& message) {
    broadcastMessage(message, "");
}

void P2PNode::broadcastMessage(const Message& message, const std::string& exclude_node) {
    std::string message_id = generateMessageId(message);
    
    // 检查是否已经处理过该消息
    {
        std::lock_guard<std::mutex> lock(processed_messages_mutex_);
        if (processed_messages_.find(message_id) != processed_messages_.end()) {
            return;  // 已经处理过，不再广播
        }
        processed_messages_.insert(message_id);
    }
    
    // 广播给除发送节点外的所有节点
    std::lock_guard<std::mutex> lock(queue_mutex_);
    for (const auto& [nodeId, socket] : connections_) {
        if (nodeId != exclude_node) {
            sendToNode(nodeId, message);
        }
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
            std::string blockHash = newBlock.getHash();
            
            // 检查区块是否已经在链上
            auto existingBlock = findBlockByHash(blockHash);
            if (existingBlock) {
                std::cout << "Block already exists in chain, ignoring new block: " << blockHash << std::endl;
                break;
            }
            
            // 检查是否已经在投票中
            {
                std::lock_guard<std::mutex> lock(consensus_mutex_);
                if (consensus_votes_.find(blockHash) != consensus_votes_.end()) {
                    std::cout << "Block is already in consensus voting: " << blockHash << std::endl;
                    break;
                }
            }
            
            // 验证区块
            if (!blockchain_->verifyBlock(newBlock)) {
                std::cout << "Block verification failed" << std::endl;
                break;
            }
            
            // 检查区块是否已经在链上
            existingBlock = findBlockByHash(blockHash);
            if (existingBlock) {
                std::cout << "Block already exists in chain" << std::endl;
                break;
            }
            
            // 发起共识投票
            std::cout << "Initiating consensus vote for block: " << blockHash << std::endl;
            broadcastConsensusVote(newBlock, true);  // 发起投票，默认投赞成票
            
            // 广播给其他节点
            broadcastMessage(message, sender);
            break;
        }
        
        case MessageType::NEW_TRANSACTION: {
            // 处理新交易
            std::cout << "  " << host_ << ":" << port_ << " Received new transaction from: " << sender << std::endl;
            json txData = json::parse(message.data);
            Transaction newTx(txData);
            if (blockchain_->addTransactionToPool(newTx)) {
                // 广播给其他节点，但排除发送节点
                broadcastMessage(message, sender);
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
                blockchain_->addBlock(block.getTransactions(), false);
            }
            break;
        }
        
        case MessageType::GET_PEERS: {
            // 发送已知节点列表
            std::cout << "  " << host_ << ":" << port_ << " Sending peers to: " << sender << std::endl;
            Message response;
            response.type = MessageType::PEERS;
            
            // 构建可连接的节点列表
            json peersArray = json::array();
            for (const auto& [nodeId, socket] : connections_) {
                // 解析节点ID
                size_t colonPos = nodeId.find(':');
                if (colonPos == std::string::npos) continue;
                
                std::string host = nodeId.substr(0, colonPos);
                int port = std::stoi(nodeId.substr(colonPos + 1));
                
                // 检查是否是本地节点
                if (host == host_ && port == port_) {
                    continue;
                }
                
                // 检查是否是主动连接的节点
                if (socket->remote_endpoint().port() != port) {
                    continue;  // 跳过使用临时端口的连接
                }
                
                // 添加到可连接节点列表
                json peerInfo = {
                    {"host", host},
                    {"port", port}
                };
                peersArray.push_back(peerInfo);
            }
            
            response.data = peersArray.dump();
            sendToNode(sender, response);
            break;
        }
        
        case MessageType::PEERS: {
            // 处理接收到的节点列表
            std::cout << "  " << host_ << ":" << port_ << " Received peers from: " << sender << std::endl;
            json peers = json::parse(message.data);
            
            // 获取当前已连接的节点列表
            auto currentNodes = getConnectedNodes();
            std::set<std::string> currentNodeSet(currentNodes.begin(), currentNodes.end());
            
            // 遍历新收到的节点列表
            for (const auto& peer : peers) {
                std::string host = peer["host"];
                int port = peer["port"];
                std::string nodeId = host + ":" + std::to_string(port);
                
                // 检查是否是本地节点
                if (host == host_ && port == port_) {
                    continue;  // 跳过本地节点
                }
                
                // 检查是否已经连接
                if (currentNodeSet.find(nodeId) != currentNodeSet.end()) {
                    continue;  // 跳过已连接的节点
                }
                
                // 尝试连接新节点
                try {
                    connect(host, port);
                } catch (const std::exception& e) {
                    std::cerr << "Failed to connect to peer " << nodeId << ": " << e.what() << std::endl;
                }
            }
            break;
        }
        
        case MessageType::GET_UTXOS: {
            handleUTXOsRequest(message, sender);
            break;
        }
        
        case MessageType::UTXOS: {
            json utxosData = json::parse(message.data);
            std::string address = utxosData["address"];
            std::vector<UTXO> utxos;
            
            // 解析UTXO数据
            for (const auto& utxoJson : utxosData["utxos"]) {
                UTXO utxo(utxoJson);
                utxos.push_back(utxo);
            }
            
            // 更新本地UTXO集合
            blockchain_->updateUTXOs(address, utxos);
            break;
        }
        
        case MessageType::GET_BALANCE: {
            handleBalanceRequest(message, sender);
            break;
        }
        
        case MessageType::BALANCE: {
            json balanceData = json::parse(message.data);
            std::string address = balanceData["address"];
            double balance = balanceData["balance"];
            
            // 更新本地余额缓存
            blockchain_->updateBalance(address, balance);
            break;
        }
        
        case MessageType::SYNC_REQUEST: {
            handleSyncRequest(message, sender);
            break;
        }
        
        case MessageType::SYNC_RESPONSE: {
            json syncData = json::parse(message.data);
            
            // 处理区块数据
            for (const auto& blockData : syncData["blocks"]) {
                Block block(blockData);
                blockchain_->addBlock(block.getTransactions(), false);
            }
            
            // 处理UTXO数据
            if (syncData.contains("utxos")) {
                for (const auto& utxoData : syncData["utxos"]) {
                    UTXO utxo(utxoData);
                    blockchain_->updateUTXO(utxo);
                }
            }
            
            // 处理待处理交易
            if (syncData.contains("pending_transactions")) {
                for (const auto& txData : syncData["pending_transactions"]) {
                    Transaction tx(txData);
                    blockchain_->addTransactionToPool(tx);
                }
            }
            
            // 更新节点状态
            if (syncData.contains("node_state")) {
                updateNodeState(syncData["node_state"]);
            }
            break;
        }
        
        case MessageType::MINING_REQUEST: {
            handleMiningRequest(message, sender);
            break;
        }
        
        case MessageType::MINING_RESPONSE: {
            json miningData = json::parse(message.data);
            // std::string blockHash = miningData["block_hash"];  冗余行，但需要确认这个响应是否携带了这个字段
            bool success = miningData["success"];
            
            if (success) {
                // 验证新区块
                Block newBlock(miningData["block"]);
                if (blockchain_->verifyBlock(newBlock)) {
                    blockchain_->addBlock(newBlock.getTransactions(), false);
                    // 广播新区块
                    Message msg;
                    msg.type = MessageType::NEW_BLOCK;
                    msg.data = newBlock.toJson();
                    broadcastMessage(msg);
                }
            }
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
        cleanupProcessedMessages();
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
    broadcastMessage(msg);
}

void P2PNode::requestBalance(const std::string& address) {
    Message msg;
    msg.type = MessageType::GET_BALANCE;
    msg.data = json({{"address", address}}).dump();
    broadcastMessage(msg);
}

void P2PNode::requestSync(int startHeight) {
    Message msg;
    msg.type = MessageType::SYNC_REQUEST;
    msg.data = json({{"start_height", startHeight}}).dump();
    broadcastMessage(msg);
}

void P2PNode::requestMining(const std::vector<Transaction>& transactions) {
    Message msg;
    msg.type = MessageType::MINING_REQUEST;
    json txArray = json::array();
    for (const auto& tx : transactions) {
        txArray.push_back(json::parse(tx.toJson()));
    }
    msg.data = txArray.dump();
    broadcastMessage(msg);
}

void P2PNode::broadcastConsensusVote(const Block& block, bool vote) {
    Message msg;
    msg.type = MessageType::CONSENSUS_VOTE;
      
    // 构建投票数据
    json voteData = {
        {"block_hash", block.getHash()},
        {"vote", vote},
        {"block", json::parse(block.toJson())},  // 添加完整的区块数据
        {"voterId", host_ + ":" + std::to_string(port_)}  // 添加投票者ID
    };
    
    msg.data = voteData.dump();
    msg.sender = host_ + ":" + std::to_string(port_);  // 设置发送者为当前节点
    
    // 首先检查区块是否已经在链上。好像有点冗余
    auto existingBlock = findBlockByHash(block.getHash());
    if (existingBlock) {
        std::cout << "Block already exists in chain, ignoring vote: " << block.getHash() << std::endl;
        return;
    }
    
    // 先处理自己的投票
    handleConsensusVote(msg, msg.sender);
    
    // 然后广播给其他节点
    broadcastMessage(msg, msg.sender);
}

void P2PNode::broadcastConsensusResult(const Block& block, bool accepted) {
    Message msg;
    msg.type = MessageType::CONSENSUS_RESULT;
        
    // 构建共识结果数据
    json resultData = {
        {"block_hash", block.getHash()},
        {"accepted", accepted},
        {"block", json::parse(block.toJson())},  // 添加完整的区块数据
        {"voterId", host_ + ":" + std::to_string(port_)}  // 添加投票者ID
    };
    
    msg.data = resultData.dump();
    broadcastMessage(msg);
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
    bool includeUtxos = request["include_utxos"];
    bool includePendingTxs = request["include_pending_txs"];
    
    // 构建同步响应
    Message response;
    response.type = MessageType::SYNC_RESPONSE;
    
    json syncData = {
        {"blocks", json::array()},
        {"utxos", json::array()},
        {"pending_transactions", json::array()},
        {"node_state", {
            {"height", blockchain_->getChain().size()},
            {"difficulty", blockchain_->getDifficulty()},
            {"version", "1.0"},
            {"last_block_hash", blockchain_->getLastBlock()->getHash()}
        }}
    };
    
    // 添加区块数据
    auto blocks = blockchain_->getBlocksFromHeight(startHeight);
    for (const auto& block : blocks) {
        syncData["blocks"].push_back(json::parse(block.toJson()));
    }
    
    // 如果需要UTXO数据
    if (includeUtxos) {
        auto utxos = blockchain_->getAllUTXOs();
        for (const auto& utxo : utxos) {
            syncData["utxos"].push_back(json::parse(utxo.toJson()));
        }
    }
    
    // 如果需要待处理交易
    if (includePendingTxs) {
        auto pendingTxs = blockchain_->getPendingTransactions();
        for (const auto& tx : pendingTxs) {
            syncData["pending_transactions"].push_back(json::parse(tx.toJson()));
        }
    }
    
    response.data = syncData.dump();
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
    blockchain_->addBlock(transactions, true);
    
    // 构建响应
    Message response;
    response.type = MessageType::MINING_RESPONSE;
    response.data = blockchain_->getChain().back()->toJson();
    
    sendToNode(sender, response);
}

void P2PNode::handleConsensusVote(const Message& message, const std::string& sender) {
    json voteData = json::parse(message.data);
    std::string blockHash = voteData["block_hash"];
    
    // 首先检查区块是否已经在链上
    auto existingBlock = findBlockByHash(blockHash);
    if (existingBlock) {
        std::cout << "Block already exists in chain, ignoring vote: " << blockHash << std::endl;
        return;
    }
    
    bool vote = voteData["vote"];
    const std::string& voterId = voteData["voterId"];
    const std::string& block = voteData["block"];

    // 验证区块哈希
    Block newBlock(block);
    if (newBlock.getHash() != blockHash) {
        std::cout << "Block hash mismatch in vote message" << std::endl;
        return;
    }

    // 记录已投票节点
    {
        std::lock_guard<std::mutex> lock(consensus_mutex_);
        if (voted_nodes_[blockHash].find(voterId) == voted_nodes_[blockHash].end()) {
            voted_nodes_[blockHash].insert(voterId);
        } else {
            std::cout << "Already voted for block: " << blockHash << std::endl;
            return;
        }
    }

    // 检查是否已投票
    {
        std::lock_guard<std::mutex> lock(consensus_mutex_);
        if (voted_blocks_.find(blockHash) != voted_blocks_.end()) {
            std::cout << "Already voted for block: " << blockHash << std::endl;
            isVoted = true;
        } else  {
            if (!blockchain_->verifyBlock(newBlock)) {
                std::cout << "Block verification failed for vote: " << blockHash << std::endl;
                broadcastConsensusResult(newBlock, false);
            }else{
                broadcastConsensusResult(newBlock, true);
                std::cout << "Block verification passed for vote: " << blockHash << std::endl;
            }
            voted_blocks_[blockHash] = true; 
        }
    }
    
    // 记录投票
    {
        std::lock_guard<std::mutex> lock(consensus_mutex_);
        // 初始化投票记录
        if (consensus_votes_.find(blockHash) == consensus_votes_.end()) {
            consensus_votes_[blockHash] = std::make_pair(0, 0);
        } 

        // 记录投票
        if (vote) {
            consensus_votes_[blockHash].first++;
        } else {
            consensus_votes_[blockHash].second++;
        }
    }

    // 检查共识
    auto votes = consensus_votes_[blockHash];
    int totalVotes = votes.first + votes.second;
    
    // 检查是否达到共识阈值
    if (totalVotes >= getMinConsensusNodes()) {
        float approvalRate = (float)votes.first / totalVotes;
        if (approvalRate >= CONSENSUS_THRESHOLD) {
            std::cout << "Consensus reached for block: " << blockHash << std::endl;
            // blockchain_->addBlock(newBlock.getTransactions(), false);
            // 发起共识结果
            broadcastConsensusResult(newBlock, true);
        } else {
            // 发起共识结果
            std::cout << "Consensus not reached for block: " << blockHash << std::endl;
            broadcastConsensusResult(newBlock, false);
        }
    }
}

void P2PNode::handleConsensusResult(const Message& message, const std::string& sender) {
    json resultData = json::parse(message.data);
    std::string blockHash = resultData["block_hash"];
    bool accepted = resultData["accepted"];
    std::string block = resultData["block"];
    std::string voterId = resultData["voterId"];
    
    if (accepted) {
        // 检查区块是否已经在链上
        auto existingBlock = findBlockByHash(blockHash);
        if (existingBlock) {
            std::cout << "Block already exists in chain: " << blockHash << std::endl;
        }else if (!resultData.contains("block")) {// 从共识结果中获取区块数据
            std::cout << "Block data not found in consensus result" << std::endl;
        }else{
            // 创建新区块
            Block newBlock(resultData["block"]);
        
            // 验证区块
            if (!blockchain_->verifyBlock(newBlock)) {
                std::cout << "Block verification failed: " << blockHash << std::endl;
            }else{
                blockchain_->addBlock(newBlock.getTransactions(), false);// 添加到区块链
                std::cout << "New block added to chain after consensus: " << blockHash << std::endl;
            }
        }         
    } else {
        std::cout << "Block rejected by consensus: " << blockHash << std::endl;
    }
    
    // 清理投票记录
    std::lock_guard<std::mutex> lock(consensus_mutex_);
    consensus_votes_.erase(blockHash);
    voted_blocks_.erase(blockHash);
    voted_nodes_.erase(blockHash);
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

std::string P2PNode::generateMessageId(const Message& message) {
    // 根据消息类型和数据生成唯一ID
    return std::to_string(static_cast<int>(message.type)) + "_" + message.data;
}

void P2PNode::cleanupProcessedMessages() {
    std::lock_guard<std::mutex> lock(processed_messages_mutex_);
    // 保留最近1000条消息记录
    if (processed_messages_.size() > 1000) {
        processed_messages_.clear();
    }
}

void P2PNode::updateNodeState(const json& state) {
    std::lock_guard<std::mutex> lock(node_state_mutex_);
    
    // 更新节点状态
    if (state.contains("height")) {
        node_state_.height = state["height"];
    }
    if (state.contains("difficulty")) {
        node_state_.difficulty = state["difficulty"];
    }
    if (state.contains("version")) {
        node_state_.version = state["version"];
    }
    if (state.contains("last_block_hash")) {
        node_state_.lastBlockHash = state["last_block_hash"];
    }
    
    std::cout << "Node state updated: height=" << node_state_.height 
              << ", difficulty=" << node_state_.difficulty 
              << ", version=" << node_state_.version 
              << ", lastBlockHash=" << node_state_.lastBlockHash << std::endl;
}

std::shared_ptr<Block> P2PNode::findBlockByHash(const std::string& blockHash) const {
    // 从区块链中查找区块
    auto chain = blockchain_->getChain();
    for (const auto& block : chain) {
        if (block->getHash() == blockHash) {
            return block;
        }
    }
    return nullptr;
}

std::shared_ptr<Block> P2PNode::findBlockByHeight(int height) const {
    // 从区块链中查找指定高度的区块
    auto chain = blockchain_->getChain();
    if (height >= 0 && height < chain.size()) {
        return chain[height];
    }
    return nullptr;
}


int P2PNode::getMinConsensusNodes() const {
    // 根据节点数量计算最小共识节点数
    return static_cast<int>(connections_.size() * 0.5) + 1;
}
