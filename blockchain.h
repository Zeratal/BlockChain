#pragma once

#include "block.h"
#include "wallet.h"
#include <vector>
#include <memory>
#include "transaction.h"
#include <map>

class Blockchain {
public:
    Blockchain(int difficulty, const std::map<std::string, double>& initialBalances);
    
    void addBlock(const std::vector<Transaction>& transactions);
    bool isChainValid() const;
    const std::vector<std::shared_ptr<Block>>& getChain() const { return chain_; }
    
    bool validateTransaction(const Transaction& tx) const;
    double getBalance(const std::string& address) const;
    
private:
    std::vector<std::shared_ptr<Block>> chain_;
    int difficulty_;
    
    std::shared_ptr<Block> createGenesisBlock();
}; 