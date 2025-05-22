#include <iostream>
#include "p2p_node.h"
#include <windows.h>

void runNode(const std::string& host, int port) {
    try {
        // 创建区块链，设置难度为 4
        auto blockchain = std::make_shared<Blockchain>(4);
        P2PNode node(host, port, blockchain);
        
        node.start();
        node.startIPC();  // 启动IPC
        
        std::cout << "Starting node at " << host << ":" << port << std::endl;
        std::cout << "Node is ready for connections" << std::endl;  // 添加这行

        std::cout << "\nAvailable commands:" << std::endl;
        std::cout << "  connect <host> <port> - Connect to a node" << std::endl;
        std::cout << "  mine - Mine a new block" << std::endl;
        std::cout << "  balance <address> - Check balance" << std::endl;
        std::cout << "  send <from> <to> <amount> - Send transaction" << std::endl;
        std::cout << "  peers - List connected peers" << std::endl;
        std::cout << "  chain - Show blockchain" << std::endl;
        std::cout << "  exit - Stop the node" << std::endl;
        
        // 修改主循环，检查退出请求
        std::string command;
        while (!node.isExitRequested()) {
            std::cout << "> ";
            std::getline(std::cin, command);
            
            if (command == "exit") {
                break;
            }
            
            // 解析命令
            std::istringstream iss(command);
            std::string cmd;
            iss >> cmd;
            
            if (cmd == "connect") {
                std::string peerHost;
                int peerPort;
                iss >> peerHost >> peerPort;
                std::cout << "Connecting to peer " << peerHost << ":" << peerPort << std::endl;
                node.connect(peerHost, peerPort);
            }
            else if (cmd == "mine") {
                // 获取待处理交易
                auto pendingTxs = blockchain->getPendingTransactions();
                if (pendingTxs.empty()) {
                    std::cout << "No pending transactions to mine" << std::endl;
                    continue;
                }
                
                // 创建新区块，使用待处理交易
                blockchain->addBlock(pendingTxs, true);
                std::cout << "New block mined" << std::endl;
                
                // 广播新区块
                Message msg;
                msg.type = MessageType::NEW_BLOCK;
                msg.data = blockchain->getChain().back()->toJson();
                node.broadcast(msg);
            }
            else if (cmd == "balance") {
                std::string address;
                iss >> address;
                double balance = blockchain->getBalance(address);
                std::cout << "Balance: " << balance << std::endl;
            }
            else if (cmd == "send") {
                std::string from, to;
                double amount;
                iss >> from >> to >> amount;
                
                // 获取发送者钱包
                auto wallet = blockchain->getWalletByPublicKey(from);
                if (!wallet) {
                    std::cout << "Wallet not found" << std::endl;
                    continue;
                }
                
                // 创建交易
                Transaction tx(from, to, amount);
                
                // 签名交易
                tx.setSignature(wallet->sign(tx.toJson()));
                
                // 添加到交易池
                if (blockchain->addTransactionToPool(tx)) {
                    std::cout << "Transaction added to pool" << std::endl;
                    
                    // 广播交易
                    Message msg;
                    msg.type = MessageType::NEW_TRANSACTION;
                    msg.data = tx.toJson();
                    node.broadcast(msg);
                } else {
                    std::cout << "Failed to add transaction" << std::endl;
                }
            }
            else if (cmd == "peers") {
                auto peers = node.getConnectedNodes();
                std::cout << "Connected peers:" << std::endl;
                for (const auto& peer : peers) {
                    std::cout << "  " << peer << std::endl;
                }
            }
            else if (cmd == "chain") {
                const auto& chain = blockchain->getChain();
                std::cout << "Blockchain:" << std::endl;
                for (size_t i = 0; i < chain.size(); ++i) {
                    std::cout << "Block " << i << ":" << std::endl;
                    std::cout << "  Hash: " << chain[i]->getHash() << std::endl;
                    std::cout << "  Transactions: " << chain[i]->getTransactions().size() << std::endl;
                }
            }
            else {
                std::cout << "Unknown command" << std::endl;
            }
        }
        
        node.stopIPC();  // 停止IPC
        node.stop();
    } catch (const std::exception& e) {
        std::cerr << "Node error: " << e.what() << std::endl;
    }
}

// 创建新进程运行节点
bool createNodeProcess(const std::string& host, int port) {
    STARTUPINFO si = {sizeof(si)};
    PROCESS_INFORMATION pi;
    
    // 创建新的控制台
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOW;
    
    // 构建命令行参数
    std::string cmdLine = "BlockChain.exe " + host + " " + std::to_string(port);
    
    // 创建进程
    if (!CreateProcess(
        NULL,                   // 应用程序名
        (LPSTR)cmdLine.c_str(), // 命令行
        NULL,                   // 进程安全属性
        NULL,                   // 线程安全属性
        FALSE,                  // 继承句柄
        CREATE_NEW_CONSOLE,     // 创建新控制台
        NULL,                   // 环境变量
        NULL,                   // 当前目录
        &si,                    // 启动信息
        &pi))                   // 进程信息
    {
        std::cerr << "Failed to create process" << std::endl;
        return false;
    }
    
    // 关闭不需要的句柄
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return true;
}

bool sendExitSignal(const std::string& host, int port) {
    std::string pipe_name = "\\\\.\\pipe\\blockchain_node_" + host + "_" + std::to_string(port);
    std::cout << "Sending exit signal to " << pipe_name << std::endl;
    HANDLE pipe = CreateFileA(
        pipe_name.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );
    
    if (pipe == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create pipe" << std::endl;
        return false;
    }
    
    const char* message = "EXIT";
    DWORD bytes_written;
    bool success = WriteFile(
        pipe,
        message,
        strlen(message),
        &bytes_written,
        NULL
    );
    
    CloseHandle(pipe);
    return success;
}

int main(int argc, char* argv[]) {
    if (argc == 3) {
        // 作为节点进程运行
        std::string host = argv[1];
        int port = std::stoi(argv[2]);
        runNode(host, port);
    } else {
        // 主进程，创建三个节点
        std::cout << "Starting three nodes..." << std::endl;
        
        // 启动三个节点进程
        createNodeProcess("127.0.0.1", 8001);
        createNodeProcess("127.0.0.1", 8002);
        createNodeProcess("127.0.0.1", 8003);
        
        // 主进程等待用户输入
        std::cout << "Enter 'exit' to stop all nodes" << std::endl;
        std::string command;
        while (true) {
            std::cout << "> ";
            std::getline(std::cin, command);
            if (command == "exit") {
                // 发送退出信号给所有节点
                sendExitSignal("127.0.0.1", 8001);
                sendExitSignal("127.0.0.1", 8002);
                sendExitSignal("127.0.0.1", 8003);
                break;
            }
        }
    }
    
    return 0;
} 