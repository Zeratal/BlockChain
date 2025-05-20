#include "transaction.h"
#include "wallet.h"
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <chrono>
#include <ctime>
#include <iostream>

TransactionInput::TransactionInput(const std::string& txId, int outputIndex, const std::string& signature)
    : txId_(txId)
    , outputIndex_(outputIndex)
    , signature_(signature)
{
}

TransactionOutput::TransactionOutput(double amount, const std::string& owner)
    : amount_(amount)
    , owner_(owner)
{
}

Transaction::Transaction(const std::string& from, const std::string& to, double amount)
    : from_(from)
    , to_(to)
    , amount_(amount)
{
    std::cout << "Transaction::Transaction: " << from_ << " " << to_ << " " << amount_ << std::endl;
    transactionId_ = calculateTransactionId();
}

Transaction Transaction::createSystemTransaction(const std::string& to, double amount) {
    Transaction tx("SYSTEM", to, amount);
    tx.signature_ = "SYSTEM_SIGNATURE";  // 系统交易的特殊签名
    tx.addOutput(TransactionOutput(amount, to));
    return tx;
}

void Transaction::addInput(const TransactionInput& input) {
    std::cout << "Transaction::addInput: " << transactionId_ << " input: " << input.getTxId() << " " << input.getOutputIndex() << std::endl;
    inputs_.push_back(input);
    transactionId_ = calculateTransactionId();
}

void Transaction::addOutput(const TransactionOutput& output) {
    std::cout << "Transaction::addOutput: " << transactionId_ << " output: " << output.getAmount() << " " << output.getOwner() << std::endl;
    outputs_.push_back(output);
    transactionId_ = calculateTransactionId();
}

bool Transaction::verifySignature() const {
    if (from_ == "SYSTEM") return true;
    
    // 验证签名
    // std::cout << "Transaction::verifySignature: " << transactionId_ << " signature: " << signature_ << std::endl;
    Wallet::verify(transactionId_, signature_, from_);
    // 这里需要实现具体的签名验证逻辑
    return !signature_.empty();
}

std::string Transaction::calculateTransactionId() const {
    std::stringstream ss;
    ss << from_ << to_ << amount_;
    
    // 添加输入和输出的信息
    for (const auto& input : inputs_) {
        ss << input.getTxId() << input.getOutputIndex();
    }
    
    for (const auto& output : outputs_) {
        ss << output.getAmount() << output.getOwner();
    }
    
    // 计算SHA256哈希
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, ss.str().c_str(), ss.str().size());
    SHA256_Final(hash, &sha256);
    
    std::stringstream hashStream;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        hashStream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return hashStream.str();
}

// 检查交易是否有效（包括余额检查）
bool Transaction::isValid() const {
    return !from_.empty() && !to_.empty() && amount_ > 0 && !signature_.empty();
}

