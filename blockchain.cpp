#include "blockchain.h"
#include <iostream>
#include <memory>
#include <vector>

// Blockchain 类实现
Blockchain::Blockchain(int difficulty)
    : difficulty_(difficulty)
{
	std::cout << "Blockchain::Blockchain createGenesisBlock" << std::endl;
    chain_.push_back(createGenesisBlock());
}

std::shared_ptr<Block> Blockchain::createGenesisBlock() {
    return std::make_shared<Block>(0, "Genesis Block", "0");
}

void Blockchain::addBlock(const std::string& data) {
    std::cout << "Blockchain::Add Block: " << data << std::endl;

    // 获取区块链中最后一个区块
    auto previousBlock = chain_.back();
    // 创建一个新的区块
    auto newBlock = std::make_shared<Block>(
        previousBlock->getIndex() + 1,
        data,
        previousBlock->getHash()
    );
    newBlock->mineBlock(difficulty_);
    chain_.push_back(newBlock);
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