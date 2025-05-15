#pragma once

#include "block.h"
#include <vector>
#include <memory>

class Blockchain {
public:
    Blockchain(int difficulty = 4);
    
    void addBlock(const std::string& data);
    bool isChainValid() const;
    const std::vector<std::shared_ptr<Block>>& getChain() const { return chain_; }
    
private:
    std::vector<std::shared_ptr<Block>> chain_;
    int difficulty_;
    
    std::shared_ptr<Block> createGenesisBlock();
}; 