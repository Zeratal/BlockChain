#include "blockchain.h"
#include "wallet.h"
#include <iostream>
#include <chrono>
#include <map>

int main() {
    try {
        // 创建钱包
        std::cout << "Creating wallets..." << std::endl;
        Wallet aliceWallet;
        Wallet bobWallet;
        Wallet charlieWallet;
        
        // 生成密钥对
        aliceWallet.generateKeyPair();
        bobWallet.generateKeyPair();
        charlieWallet.generateKeyPair();
        
        // 获取公钥
        std::string alicePublicKey = aliceWallet.getPublicKey();
        std::string bobPublicKey = bobWallet.getPublicKey();
        std::string charliePublicKey = charlieWallet.getPublicKey();
        
        std::cout << "Alice's public key: " << alicePublicKey << std::endl;
        std::cout << "Bob's public key: " << bobPublicKey << std::endl;
        std::cout << "Charlie's public key: " << charliePublicKey << std::endl;
		
        // 设置初始余额
        std::map<std::string, double> balances;
        balances[alicePublicKey] = 100.0;
        balances[bobPublicKey] = 50.0;
        balances[charliePublicKey] = 25.0;
        
        // 打印初始余额
        std::cout << "\nInitial balances:" << std::endl;
        std::cout << "Alice: " << balances[alicePublicKey] << std::endl;
        std::cout << "Bob: " << balances[bobPublicKey] << std::endl;
        std::cout << "Charlie: " << balances[charliePublicKey] << std::endl;
        
        // 创建交易
        std::cout << "\nCreating transactions..." << std::endl;
        std::vector<Transaction> transactions1 = {
            Transaction(alicePublicKey, bobPublicKey, 10.0),
            Transaction(bobPublicKey, charliePublicKey, 5.0),
            Transaction(charliePublicKey, alicePublicKey, 2.5)
        };
        
        // 检查余额
        for (const auto& tx : transactions1) {
            if (balances[tx.getFrom()] < tx.getAmount()) {
                throw std::runtime_error("Insufficient balance for transaction");
            }
        }
        
                
        // 检查余额
        for (const auto& tx : transactions1) {
            if (!tx.hasEnoughBalance(balances[tx.getFrom()])) {
                throw std::runtime_error("Insufficient balance for transaction");
            }
        }
        // 签名交易
        transactions1[0].setSignature(aliceWallet.sign(transactions1[0].getTransactionId()));
        transactions1[1].setSignature(bobWallet.sign(transactions1[1].getTransactionId()));
        transactions1[2].setSignature(charlieWallet.sign(transactions1[2].getTransactionId()));
        
        // 验证交易签名
        std::cout << "\nVerifying transaction signatures..." << std::endl;
        for (const auto& tx : transactions1) {
            std::cout << "Transaction " << tx.getTransactionId() << " signature valid: " 
                      << (tx.verifySignature() ? "Yes" : "No") << std::endl;
        }

        

        // 更新余额
        for (const auto& tx : transactions1) {
            balances[tx.getFrom()] -= tx.getAmount();
            balances[tx.getTo()] += tx.getAmount();
        }
        
        // 创建第二个交易集合
        std::cout << "\nCreating second transaction set..." << std::endl;
        std::vector<Transaction> transactions2 = {
            Transaction(alicePublicKey, charliePublicKey, 7.5),
            Transaction(charliePublicKey, bobPublicKey, 3.0)
        };
        
        // 检查余额
        for (const auto& tx : transactions2) {
            if (balances[tx.getFrom()] < tx.getAmount()) {
                throw std::runtime_error("Insufficient balance for transaction");
            }
        }
        
        // 签名第二个交易集合
        transactions2[0].setSignature(aliceWallet.sign(transactions2[0].getTransactionId()));
        transactions2[1].setSignature(charlieWallet.sign(transactions2[1].getTransactionId()));
        
                // 检查余额
        for (const auto& tx : transactions2) {
            if (!tx.hasEnoughBalance(balances[tx.getFrom()])) {
                throw std::runtime_error("Insufficient balance for transaction");
            }
        }
        // 更新余额
        for (const auto& tx : transactions2) {
            balances[tx.getFrom()] -= tx.getAmount();
            balances[tx.getTo()] += tx.getAmount();
        }
        
        // 创建一个难度为4的区块链
        std::cout << "\nCreating blockchain..." << std::endl;
        Blockchain blockchain(4);
        
        std::cout << "\nStarting mining..." << std::endl;
        
        // 添加区块
        blockchain.addBlock(transactions1);
        blockchain.addBlock(transactions2);
        
        // 验证区块链
        std::cout << "\nIs blockchain valid: " << (blockchain.isChainValid() ? "Yes" : "No") << std::endl;
        
        // 打印最终余额
        std::cout << "\nFinal balances:" << std::endl;
        std::cout << "Alice: " << balances[alicePublicKey] << std::endl;
        std::cout << "Bob: " << balances[bobPublicKey] << std::endl;
        std::cout << "Charlie: " << balances[charliePublicKey] << std::endl;
        
        // 打印区块链信息
        std::cout << "\nBlockchain Information:" << std::endl;
        for (const auto& block : blockchain.getChain()) {
            std::cout << "\n  ------------" << "Index: " << block->getIndex() << "------------" << std::endl;
            std::cout << "  Timestamp: " << block->getTimestamp() << std::endl;
            std::cout << "  Merkle Root: " << block->getMerkleRoot() << std::endl;
            std::cout << "  Previous Hash: " << block->getPreviousHash() << std::endl;
            std::cout << "  Hash: " << block->getHash() << std::endl;
            std::cout << "  Nonce: " << block->getNonce() << std::endl;
            
            std::cout << "\n  Transactions:" << std::endl;
            for (const auto& tx : block->getTransactions()) {
                std::cout << "    ------------------------" << std::endl;
                std::cout << "    From: " << tx.getFrom() << std::endl;
                std::cout << "    To: " << tx.getTo() << std::endl;
                std::cout << "    Amount: " << tx.getAmount() << std::endl;
                std::cout << "    Transaction ID: " << tx.getTransactionId() << std::endl;
                std::cout << "    Signature Valid: " << (tx.verifySignature() ? "Yes" : "No") << std::endl;
            }
            
            // 在添加区块后打印 Merkle 树结构
            std::cout << "\n  Merkle Tree for Block " << block->getIndex() << ":" << std::endl;
            MerkleTree tree(block->getTransactions());
            tree.printTree();
            
            // 验证每个交易
            std::cout << "\n  Verifying transactions..." << std::endl;
            for (const auto& tx : block->getTransactions()) {
                std::cout << "    Transaction " << tx.getTransactionId() << " is " 
                          << (tree.verifyTransaction(tx) ? "valid" : "invalid") << std::endl;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        getchar();
        return 1;
    }
    getchar();
    return 0;
} 