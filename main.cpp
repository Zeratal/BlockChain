#include "blockchain.h"
#include "transaction.h"
#include <iostream>
#include <chrono>

int main() {
    // Create a blockchain with difficulty 4
    Blockchain blockchain(4);
    
    std::cout << "Starting mining..." << std::endl;
    
    // Create some transactions
    std::vector<Transaction> transactions1 = {
        Transaction("Alice", "Bob", 10.0),
        Transaction("Bob", "Charlie", 5.0),
        Transaction("Charlie", "Alice", 2.5)
    };
    
    std::vector<Transaction> transactions2 = {
        Transaction("David", "Eve", 7.5),
        Transaction("Eve", "Frank", 3.0)
    };
    
    // Add blocks with transactions
    blockchain.addBlock(transactions1);
    blockchain.addBlock(transactions2);
    
    // Validate the blockchain
    std::cout << "\nIs blockchain valid: " << (blockchain.isChainValid() ? "Yes" : "No") << std::endl;
    
    // Print blockchain information
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
        }
        std::cout << "  ------------" << "Index: " << block->getIndex() << "------------" << std::endl;
    }
    getchar();
    return 0;
} 