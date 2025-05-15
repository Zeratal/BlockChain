#pragma once

#include "block.h"
#include "wallet.h"
#include <vector>
#include <memory>
#include "transaction.h"
#include <map>

class Blockchain {
public:
    Blockchain(int difficulty);
    
    void addBlock(const std::vector<Transaction>& transactions);
    bool isChainValid() const;
    const std::vector<std::shared_ptr<Block>>& getChain() const { return chain_; }
    
    bool validateTransaction(const Transaction& tx) const;
    void updateBalances(const std::vector<Transaction>& transactions);
    std::shared_ptr<Wallet> getWalletByPublicKey(const std::string& publicKey) const;
    
private:
    std::vector<std::shared_ptr<Block>> chain_;
    int difficulty_;
    std::map<std::string, std::shared_ptr<Wallet>> wallets_;
    
    std::shared_ptr<Block> createGenesisBlock();
}; 