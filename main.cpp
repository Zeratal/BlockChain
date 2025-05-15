#include "blockchain.h"
#include <iostream>
#include <chrono>

int main() {
    // Create a blockchain with difficulty 4
    Blockchain blockchain(4);
    
    std::cout << "Starting mining..." << std::endl;
    
    // Create some transactions
    blockchain.addBlock("First Block    ");
    blockchain.addBlock("Second Block");
    blockchain.addBlock("Third Block");
    

    // Validate the blockchain
    std::cout << "\nIs blockchain valid: " << (blockchain.isChainValid() ? "Yes" : "No") << std::endl;
    
    // Print blockchain information
    std::cout << "\nBlockchain Information:" << std::endl;
    for (const auto& block : blockchain.getChain()) {
        std::cout << "Index: " << block->getIndex() << std::endl;
        std::cout << "Timestamp: " << block->getTimestamp() << std::endl;
        std::cout << "Previous Hash: " << block->getPreviousHash() << std::endl;
        std::cout << "Hash: " << block->getHash() << std::endl;
        std::cout << "Nonce: " << block->getNonce() << std::endl;

        std::cout << "------------------------" << std::endl;
    }
    getchar();
    return 0;
} 