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
#include <set>
#include <boost/asio.hpp>
#include "blockchain.h"
#include "transaction.h"
#include <unordered_set>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
using boost::asio::ip::tcp;

// 消息类型枚举
enum class MessageType {
    HANDSHAKE,           // 握手消息
    NEW_BLOCK,          // 新区块
    NEW_TRANSACTION,    // 新交易
    GET_BLOCKS,         // 请求区块
    BLOCKS,             // 区块数据
    GET_PEERS,          // 请求节点列表
    PEERS,              // 节点列表
    GET_UTXOS,          // 请求UTXO数据
    UTXOS,              // UTXO数据
    GET_BALANCE,        // 请求余额
    BALANCE,            // 余额数据
    SYNC_REQUEST,       // 同步请求
    SYNC_RESPONSE,      // 同步响应
    MINING_REQUEST,     // 挖矿请求
    MINING_RESPONSE,    // 挖矿响应
    CONSENSUS_VOTE,     // 共识投票
    CONSENSUS_RESULT    // 共识结果
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

    // 新增的区块链网络操作方法
    void requestUTXOs(const std::string& address);
    void requestBalance(const std::string& address);
    void requestSync(int startHeight);
    void requestMining(const std::vector<Transaction>& transactions);
    void broadcastConsensusVote(const std::string& blockHash, bool vote);
    void broadcastConsensusResult(const std::string& blockHash, bool accepted);

    // 添加IPC相关方法
    void startIPC();
    void stopIPC();
    bool isExitRequested() const;

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

    // 新增的消息处理方法
    void handleUTXOsRequest(const Message& message, const std::string& sender);
    void handleBalanceRequest(const Message& message, const std::string& sender);
    void handleSyncRequest(const Message& message, const std::string& sender);
    void handleMiningRequest(const Message& message, const std::string& sender);
    void handleConsensusVote(const Message& message, const std::string& sender);
    void handleConsensusResult(const Message& message, const std::string& sender);

    // IPC相关成员
    std::atomic<bool> exit_requested_{false};
    std::thread ipc_thread_;
    HANDLE pipe_handle_{INVALID_HANDLE_VALUE};
    
    void ipcLoop();

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
    std::thread io_thread_;  // 添加 IO 线程

    // 添加消息去重集合
    std::unordered_set<std::string> processed_messages_;
    std::mutex processed_messages_mutex_;
    
    // 添加消息ID生成方法
    std::string generateMessageId(const Message& message);
    
    // 修改广播方法
    void cleanupProcessedMessages();
    void broadcastMessage(const Message& message, const std::string& exclude_node = "");

    // 添加节点状态相关成员
    struct NodeState {
        int height;
        int difficulty;
        std::string version;
        std::string lastBlockHash;
    };
    NodeState node_state_;
    std::mutex node_state_mutex_;
    
    std::mutex consensus_mutex_;
    std::map<std::string, std::pair<int, int>> consensus_votes_;

    // 添加节点状态更新方法
    void updateNodeState(const json& state);

    // 添加区块查找方法
    std::shared_ptr<Block> findBlockByHash(const std::string& blockHash) const;
    std::shared_ptr<Block> findBlockByHeight(int height) const;
    int getMinConsensusNodes() const;
}; 