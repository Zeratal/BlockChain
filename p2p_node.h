#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <functional>
#include <boost/asio.hpp>
#include "blockchain.h"
#include "transaction.h"

using boost::asio::ip::tcp;

// 消息类型枚举
enum class MessageType {
    HANDSHAKE,
    NEW_BLOCK,
    NEW_TRANSACTION,
    GET_BLOCKS,
    BLOCKS,
    GET_PEERS,
    PEERS
};

// 消息结构
struct Message {
    MessageType type;
    std::string data;
    std::string sender;
    std::string signature;
};

class P2PNode {
public:
    P2PNode(const std::string& host, int port, std::shared_ptr<Blockchain> blockchain);
    ~P2PNode();

    // 启动节点
    void start();
    // 停止节点
    void stop();
    // 连接到其他节点
    void connect(const std::string& host, int port);
    // 广播消息
    void broadcast(const Message& message);
    // 发送消息到特定节点
    void sendToNode(const std::string& nodeId, const Message& message);
    // 获取所有连接的节点
    std::vector<std::string> getConnectedNodes() const;

private:
    // 处理新连接
    void handleNewConnection();
    // 处理消息
    void handleMessage(const Message& message, const std::string& sender);
    // 消息处理循环
    void messageLoop();
    // 验证消息
    bool verifyMessage(const Message& message);
    // 序列化消息
    std::string serializeMessage(const Message& message);
    // 反序列化消息
    Message deserializeMessage(const std::string& data);

    std::string host_;
    int port_;
    std::shared_ptr<Blockchain> blockchain_;
    boost::asio::io_context io_context_;
    tcp::acceptor acceptor_;
    std::map<std::string, std::shared_ptr<tcp::socket>> connections_;
    std::queue<Message> message_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;  // 添加条件变量声明
    std::atomic<bool> running_;
    std::thread message_thread_;
}; 