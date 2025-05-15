#pragma once
#include <string>
#include <ctime>
#include <iostream>
class Transaction {
public:
    Transaction(const std::string& from, const std::string& to, double amount);
    
    // Getters
    const std::string& getFrom() const { return from_; }
    const std::string& getTo() const { return to_; }
    double getAmount() const { return amount_; }
    const std::string& getTimestamp() const { return timestamp_; }
    const std::string& getTransactionId() const { return transactionId_; }
    const std::string& getSignature() const { return signature_; }
    bool isValid() const;
    bool hasEnoughBalance(double balance) const {
        std::cout << "Checking balance: " << balance << " >= " << amount_ << std::endl;
        return balance >= amount_;
    }
   
    // 验证交易签名
    bool verifySignature() const;
    
    // 设置签名（用于从网络接收交易时）
    void setSignature(const std::string& signature) { signature_ = signature; }

private:
    std::string from_;          // 发送方公钥
    std::string to_;            // 接收方公钥
    double amount_;             // 交易金额
    std::string timestamp_;     // 交易时间戳
    std::string transactionId_; // 交易ID（哈希值）
    std::string signature_;     // 交易签名
}; 