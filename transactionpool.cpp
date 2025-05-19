#include "transactionpool.h"
#include <algorithm>

TransactionPool::TransactionPool() {
}

bool TransactionPool::addTransaction(const Transaction& transaction, const UTXOPool& utxoPool) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 检查交易是否已经在池中
    if (transactions_.find(transaction.getTransactionId()) != transactions_.end()) {
        return false;
    }
    
    // 验证交易
    if (!isValidTransaction(transaction, utxoPool)) {
        return false;
    }
    
    // 添加到交易池
    transactions_[transaction.getTransactionId()] = transaction;
    return true;
}

void TransactionPool::removeTransaction(const std::string& txId) {
    std::lock_guard<std::mutex> lock(mutex_);
    transactions_.erase(txId);
}

std::vector<Transaction> TransactionPool::getTransactions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Transaction> result;
    result.reserve(transactions_.size());
    
    for (const auto& pair : transactions_) {
        result.push_back(pair.second);
    }
    
    return result;
}

size_t TransactionPool::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return transactions_.size();
}

void TransactionPool::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    transactions_.clear();
}

bool TransactionPool::isValidTransaction(const Transaction& transaction, const UTXOPool& utxoPool) const {
    // 验证签名
    if (!transaction.verifySignature()) {
        return false;
    }
    
    // 验证发送者有足够的余额
    if (!utxoPool.hasEnoughFunds(transaction.getFrom(), transaction.getAmount())) {
        return false;
    }
    
    // 验证交易金额大于0
    if (transaction.getAmount() <= 0) {
        return false;
    }
    
    // 验证发送者和接收者不是同一个地址
    if (transaction.getFrom() == transaction.getTo()) {
        return false;
    }
    
    return true;
}

std::vector<Transaction> TransactionPool::getTransactionsForAddress(const std::string& address) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Transaction> result;
    
    for (const auto& pair : transactions_) {
        const Transaction& tx = pair.second;
        if (tx.getFrom() == address || tx.getTo() == address) {
            result.push_back(tx);
        }
    }
    
    return result;
} 