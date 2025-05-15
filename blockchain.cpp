#include "blockchain.h"
#include <iostream>
#include <memory>
#include <vector>
#include <map>

// Blockchain 类实现
Blockchain::Blockchain(int difficulty)
    : difficulty_(difficulty)
{
	std::cout << "Blockchain::Blockchain createGenesisBlock" << std::endl;
    chain_.push_back(createGenesisBlock());
}

std::shared_ptr<Block> Blockchain::createGenesisBlock() {
    std::vector<Transaction> genesisTransactions;
    return std::make_shared<Block>(0, genesisTransactions, "0");
}

void Blockchain::addBlock(const std::vector<Transaction>& transactions) {
    std::cout << "\nBlockchain::Add Block: " << transactions.size() << " transactions" << std::endl;
    // 验证所有交易
    for (const auto& tx : transactions) {
        if (!validateTransaction(tx)) {
            throw std::runtime_error("Invalid transaction: insufficient balance or invalid signature");
        }
    }
    
    // 创建新区块
    std::string previousHash = chain_.empty() ? "0" : chain_.back()->getHash();
    auto newBlock = std::make_shared<Block>(chain_.size(), transactions, previousHash);
    
    // 挖矿
    newBlock->mineBlock(difficulty_);
    
    // 添加区块
    chain_.push_back(newBlock);
    std::cout << "" << std::endl;
}

bool Blockchain::isChainValid() const {
    for (size_t i = 1; i < chain_.size(); i++) {
        const auto& currentBlock = chain_[i];
        const auto& previousBlock = chain_[i-1];
        
        if (!currentBlock->isValid()) {
            return false;
        }
        
        if (currentBlock->getPreviousHash() != previousBlock->getHash()) {
            return false;
        }
    }
    return true;
}

// 验证交易（只验证签名和基本有效性）
bool Blockchain::validateTransaction(const Transaction& tx) const {
    return tx.isValid() && tx.verifySignature();
}
