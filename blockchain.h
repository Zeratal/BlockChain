#pragma once

#include "block.h"
#include "wallet.h"
#include "utxo.h"
#include "transactionpool.h"
#include <vector>
#include <memory>
#include "transaction.h"
#include <map>
#include <mutex>

class Blockchain {
public:
    Blockchain(int difficulty);
    
    void addBlock(const std::vector<Transaction>& transactions, bool usePendingTxs);
    bool isChainValid() const;
    const std::vector<std::shared_ptr<Block>>& getChain() const { return chain_; }
    std::shared_ptr<Block>& getLastBlock() { return chain_.back(); }
    int getDifficulty() const { return difficulty_; }
    bool validateTransaction(const Transaction& tx) const;
    double getBalance(const std::string& address) const;
    // 获取从指定高度开始的所有区块
    std::vector<Block> getBlocksFromHeight(int startHeight) const;
    
    // 添加钱包管理功能
    void registerWallet(std::shared_ptr<Wallet> wallet);
    std::shared_ptr<Wallet> getWalletByPublicKey(const std::string& publicKey) const;
    
    // 新增的UTXO和交易池相关方法
    bool addTransactionToPool(const Transaction& transaction);
    std::vector<Transaction> getPendingTransactions() const;
    void updateUTXOPool(const Block& block);
    std::vector<UTXO> getUTXOsForAddress(const std::string& address) const;
    
    // 添加新的UTXO管理方法
    void updateUTXO(const UTXO& utxo);
    void updateUTXOs(const std::string& address, const std::vector<UTXO>& utxos);
    std::vector<UTXO> getAllUTXOs() const;
    
    // 添加区块验证方法
    bool verifyBlock(const Block& block) const;
    // 添加余额管理方法
    void updateBalance(const std::string& address, double balance);
    
private:
    std::vector<std::shared_ptr<Block>> chain_;
    int difficulty_;
    std::map<std::string, double> balanceCache_;  // 余额缓存
    std::map<std::string, std::shared_ptr<Wallet>> wallets_;  // 钱包映射
    
    // UTXO池和交易池
    UTXOPool utxoPool_;
    TransactionPool transactionPool_;
    
    std::map<std::string, std::vector<UTXO>> utxos_;  // 添加UTXO存储
    
    std::shared_ptr<Block> createGenesisBlock();
    void clearPendingTransactions();
    
    mutable std::map<std::string, double> balances_;  // 添加 mutable 关键字
    mutable std::mutex balances_mutex_;               // 互斥锁也需要是 mutable
}; 