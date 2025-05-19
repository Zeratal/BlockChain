#pragma once

#include "transaction.h"
#include "utxo.h"
#include <vector>
#include <map>
#include <memory>
#include <mutex>

class TransactionPool {
public:
    TransactionPool();
    
    // 添加交易到池中
    bool addTransaction(const Transaction& transaction, const UTXOPool& utxoPool);
    
    // 从池中移除交易
    void removeTransaction(const std::string& txId);
    
    // 获取池中的所有交易
    std::vector<Transaction> getTransactions() const;
    
    // 获取池中的交易数量
    size_t size() const;
    
    // 清空交易池
    void clear();
    
    // 验证交易是否有效
    bool isValidTransaction(const Transaction& transaction, const UTXOPool& utxoPool) const;
    
    // 获取指定地址的所有待处理交易
    std::vector<Transaction> getTransactionsForAddress(const std::string& address) const;
    
private:
    std::map<std::string, Transaction> transactions_; // txId -> Transaction
    mutable std::mutex mutex_;
}; 