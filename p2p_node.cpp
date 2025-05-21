#include "p2p_node.h"
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>

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
    running_ = true;
    message_thread_ = std::thread(&P2PNode::messageLoop, this);
    
    // 开始接受新连接
    handleNewConnection();
    
    // 运行IO服务
    io_context_.run();
}

void P2PNode::stop() {
    running_ = false;
    if (message_thread_.joinable()) {
        message_thread_.join();
    }
    io_context_.stop();
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
                        }
                    });
            }
            handleNewConnection();
        });
}

void P2PNode::handleMessage(const Message& message, const std::string& sender) {
    switch (message.type) {
        case MessageType::HANDSHAKE:
            std::cout << "Received handshake from: " << sender << std::endl;
            break;
            
        case MessageType::NEW_BLOCK: {
            // 处理新区块
            json blockData = json::parse(message.data);
            Block newBlock(blockData);
            blockchain_->addBlock(newBlock.getTransactions());
            // 广播给其他节点
            broadcast(message);
            break;
        }
        
        case MessageType::NEW_TRANSACTION: {
            // 处理新交易
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
            json blocksData = json::parse(message.data);
            for (const auto& blockData : blocksData) {
                Block block(blockData);
                blockchain_->addBlock(block.getTransactions());
            }
            break;
        }
        
        case MessageType::GET_PEERS: {
            // 发送已知节点列表
            Message response;
            response.type = MessageType::PEERS;
            response.data = json(getConnectedNodes()).dump();
            sendToNode(sender, response);
            break;
        }
        
        case MessageType::PEERS: {
            // 处理接收到的节点列表
            json peers = json::parse(message.data);
            for (const auto& peer : peers) {
                std::string host = peer["host"];
                int port = peer["port"];
                connect(host, port);
            }
            break;
        }
    }
}

void P2PNode::messageLoop() {
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
    return true;
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