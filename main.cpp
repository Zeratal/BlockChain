#include "blockchain.h"
#include "wallet.h"
#include <iostream>
#include <chrono>

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
        
        // 创建交易
        std::cout << "\nCreating transactions..." << std::endl;
        std::vector<Transaction> transactions1 = {
            Transaction(alicePublicKey, bobPublicKey, 10.0),
            Transaction(bobPublicKey, charliePublicKey, 5.0),
            Transaction(charliePublicKey, alicePublicKey, 2.5)
        };
        
        // 签名交易
        transactions1[0].sign(aliceWallet.getPrivateKey());
        transactions1[1].sign(bobWallet.getPrivateKey());
        transactions1[2].sign(charlieWallet.getPrivateKey());
        
        // 验证交易签名
        std::cout << "\nVerifying transaction signatures..." << std::endl;
        for (const auto& tx : transactions1) {
            std::cout << "Transaction " << tx.getTransactionId() << " signature valid: " 
                      << (tx.verifySignature() ? "Yes" : "No") << std::endl;
        }
        
        // 创建第二个交易集合
        std::vector<Transaction> transactions2 = {
            Transaction(alicePublicKey, charliePublicKey, 7.5),
            Transaction(charliePublicKey, bobPublicKey, 3.0)
        };
        
        // 签名第二个交易集合
        transactions2[0].sign(aliceWallet.getPrivateKey());
        transactions2[1].sign(charlieWallet.getPrivateKey());
        
        // 创建一个难度为4的区块链
        std::cout << "\nCreating blockchain..." << std::endl;
        Blockchain blockchain(4);
        
        std::cout << "Starting mining..." << std::endl;
        
        // 添加区块
        blockchain.addBlock(transactions1);
        blockchain.addBlock(transactions2);
        
        // 验证区块链
        std::cout << "\nIs blockchain valid: " << (blockchain.isChainValid() ? "Yes" : "No") << std::endl;
        
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