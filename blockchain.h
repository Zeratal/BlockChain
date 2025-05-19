#pragma once

#include "block.h"
#include "wallet.h"
#include "utxo.h"
#include "transactionpool.h"
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
    double getBalance(const std::string& address) const;
    
    // 添加钱包管理功能
    void registerWallet(std::shared_ptr<Wallet> wallet);
    std::shared_ptr<Wallet> getWalletByPublicKey(const std::string& publicKey) const;
    
    // 新增的UTXO和交易池相关方法
    bool addTransactionToPool(const Transaction& transaction);
    std::vector<Transaction> getPendingTransactions() const;
    void updateUTXOPool(const Block& block);
    std::vector<UTXO> getUTXOsForAddress(const std::string& address) const;
    
private:
    std::vector<std::shared_ptr<Block>> chain_;
    int difficulty_;
    std::map<std::string, double> balanceCache_;  // 余额缓存
    std::map<std::string, std::shared_ptr<Wallet>> wallets_;  // 钱包映射
    
    // UTXO池和交易池
    UTXOPool utxoPool_;
    TransactionPool transactionPool_;
    
    std::shared_ptr<Block> createGenesisBlock();
}; 