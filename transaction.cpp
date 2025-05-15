#include "transaction.h"
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <chrono>
#include <ctime>

Transaction::Transaction(const std::string& from, const std::string& to, double amount)
    : from_(from)
    , to_(to)
    , amount_(amount)
{
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
} 