#pragma once

#include "block.h"
#include <vector>
#include <memory>
#include "transaction.h"

class Blockchain {
public:
    Blockchain(int difficulty);
    
    void addBlock(const std::vector<Transaction>& transactions);
    bool isChainValid() const;
    const std::vector<std::shared_ptr<Block>>& getChain() const { return chain_; }
    
private:
    std::vector<std::shared_ptr<Block>> chain_;
    int difficulty_;
    
    std::shared_ptr<Block> createGenesisBlock();
}; 