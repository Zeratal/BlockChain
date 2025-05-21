#include "blockchain.h"
#include "wallet.h"
#include "p2p_node.h"
#include <iostream>
#include <chrono>
#include <map>
#include <thread>
#include <vector>

void runNode(const std::string& host, int port, const std::vector<std::pair<std::string, int>>& peers) {
    try {
        // 创建区块链和钱包
        auto blockchain = std::make_shared<Blockchain>(4);
        auto wallet = std::make_shared<Wallet>();
        
        // 创建P2P节点
        auto node = std::make_shared<P2PNode>(host, port, blockchain);
        
        // 启动节点
        std::cout << "Starting node on " << host << ":" << port << std::endl;
        node->start();
        
        // 连接到其他节点
        for (const auto& [peerHost, peerPort] : peers) {
            std::cout << "Connecting to peer " << peerHost << ":" << peerPort << std::endl;
            node->connect(peerHost, peerPort);
        }
        
        // 等待用户输入
        std::string command;
        while (true) {
            std::cout << "\nEnter command (send/balance/peers/exit): ";
            std::getline(std::cin, command);
            
            if (command == "exit") {
                break;
            } else if (command == "peers") {
                auto peers = node->getConnectedNodes();
                std::cout << "Connected peers:" << std::endl;
                for (const auto& peer : peers) {
                    std::cout << "  " << peer << std::endl;
                }
            } else if (command == "balance") {
                std::cout << "Balance: " << blockchain->getBalance(wallet->getPublicKey()) << std::endl;
            } else if (command == "send") {
                std::string recipient;
                double amount;
                
                std::cout << "Enter recipient address: ";
                std::getline(std::cin, recipient);
                
                std::cout << "Enter amount: ";
                std::cin >> amount;
                std::cin.ignore();
                
                // 创建交易
                Transaction tx(wallet->getPublicKey(), recipient, amount);
                std::string signature = wallet->sign(tx.getTransactionId());
                tx.setSignature(signature);
                
                // 广播交易
                Message msg;
                msg.type = MessageType::NEW_TRANSACTION;
                msg.data = tx.toJson();
                msg.sender = host + ":" + std::to_string(port);
                node->broadcast(msg);
                
                std::cout << "Transaction broadcasted" << std::endl;
            }
        }
        
        node->stop();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int main() {
    try {
        // 创建三个节点
        std::vector<std::thread> nodes;
        
        // 节点1 (localhost:8001)
        nodes.push_back(std::thread(runNode, "127.0.0.1", 8001, std::vector<std::pair<std::string, int>>{
            {"127.0.0.1", 8002},
            {"127.0.0.1", 8003}
        }));
        
        // 节点2 (localhost:8002)
        nodes.push_back(std::thread(runNode, "127.0.0.1", 8002, std::vector<std::pair<std::string, int>>{
            {"127.0.0.1", 8001},
            {"127.0.0.1", 8003}
        }));
        
        // 节点3 (localhost:8003)
        nodes.push_back(std::thread(runNode, "127.0.0.1", 8003, std::vector<std::pair<std::string, int>>{
            {"127.0.0.1", 8001},
            {"127.0.0.1", 8002}
        }));
        
        // 等待所有节点完成
        for (auto& node : nodes) {
            node.join();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 