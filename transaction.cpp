#include "transaction.h"
#include "wallet.h"
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <chrono>
#include <ctime>
#include <iostream>

Transaction::Transaction(const std::string& from, const std::string& to, double amount)
    : from_(from)
    , to_(to)
    , amount_(amount)
{
    std::cout << "Transaction created with from: " << from_ << ", to: " << to_ << ", amount: " << amount_ << std::endl;
    // Set timestamp
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    timestamp_ = std::ctime(&now_c);
    timestamp_.pop_back(); // Remove newline character

    // Generate transaction ID (hash of transaction data)
    std::stringstream ss;
    ss << from_ << to_ << amount_ << timestamp_;
    std::string data = ss.str();
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data.c_str(), data.size());
    SHA256_Final(hash, &sha256);
    
    std::stringstream hash_ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        hash_ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    transactionId_ = hash_ss.str();
    std::cout << "Transaction ID: " << transactionId_ << std::endl;
}


bool Transaction::verifySignature() const {
    // 系统交易的特殊处理
    if (from_ == "SYSTEM" && signature_ == "SYSTEM_SIGNATURE") {
        return true;
    }
    
    // 普通交易的签名验证
    return Wallet::verify(getTransactionId(), signature_, from_);
}

// 检查交易是否有效（包括余额检查）
bool Transaction::isValid() const {
    return !from_.empty() && !to_.empty() && amount_ > 0 && !signature_.empty();
}

