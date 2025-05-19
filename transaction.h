#pragma once
#include <string>
#include <ctime>
#include <iostream>
#include "wallet.h"
#include <vector>
#include <memory>

class TransactionInput {
public:
    TransactionInput(const std::string& txId, int outputIndex, const std::string& signature);
    
    const std::string& getTxId() const { return txId_; }
    int getOutputIndex() const { return outputIndex_; }
    const std::string& getSignature() const { return signature_; }
    
private:
    std::string txId_;
    int outputIndex_;
    std::string signature_;
};

class TransactionOutput {
public:
    TransactionOutput(double amount, const std::string& owner);
    
    double getAmount() const { return amount_; }
    const std::string& getOwner() const { return owner_; }
    
private:
    double amount_;
    std::string owner_;
};

class Transaction {
public:
    Transaction() : amount_(0.0) {}  // 添加默认构造函数
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

    // 创建系统交易（用于初始余额分配）
    static Transaction createSystemTransaction(const std::string& to, double amount);

    void addInput(const TransactionInput& input);
    void addOutput(const TransactionOutput& output);
    const std::vector<TransactionInput>& getInputs() const { return inputs_; }
    const std::vector<TransactionOutput>& getOutputs() const { return outputs_; }

private:
    std::string from_;          // 发送方公钥
    std::string to_;            // 接收方公钥
    double amount_;             // 交易金额
    std::string timestamp_;     // 交易时间戳
    std::string transactionId_; // 交易ID（哈希值）
    std::string signature_;     // 交易签名
    std::vector<TransactionInput> inputs_;
    std::vector<TransactionOutput> outputs_;

    std::string calculateTransactionId() const;
}; 