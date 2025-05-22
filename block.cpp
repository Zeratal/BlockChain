#include "block.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <openssl/sha.h>

using json = nlohmann::json;

// Block 类实现
Block::Block(int index, const std::vector<Transaction>& transactions, const std::string& previousHash)
    : index_(index)
    , transactions_(transactions)
    , previousHash_(previousHash)
    , nonce_(0)
{
    std::cout << "Block::Block create" << index_ << std::endl;
    // 使用高精度时间戳
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    timestamp_ = std::to_string(nanoseconds);

    // Create Merkle tree and get root hash
    MerkleTree merkleTree(transactions_);
    merkleRoot_ = merkleTree.getRootHash();
    
    hash_ = calculateHash();
}

Block::Block(const json& json) {
    index_ = json["index"];
    timestamp_ = json["timestamp"];
    previousHash_ = json["previousHash"];
    hash_ = json["hash"];
    nonce_ = json["nonce"];
    merkleRoot_ = json["merkleRoot"];
    
    // 解析交易
    for (const auto& txJson : json["transactions"]) {
        Transaction tx(txJson["from"], txJson["to"], txJson["amount"]);
        tx.setSignature(txJson["signature"]);
        transactions_.push_back(tx);
    }
    
    // 解析余额变更
    if (json.contains("balanceChanges")) {
        for (const auto& [key, value] : json["balanceChanges"].items()) {
            balanceChanges_[key] = value;
        }
    }
}

std::string Block::sha256(const std::string& str) {
    unsigned char hash[SHA256_DIGEST_LENGTH];//SHA256_DIGEST_LENGTH 是 SHA-256 哈希算法输出的哈希值长度，为 32 字节
    SHA256_CTX sha256;//SHA256_CTX 是一个结构体，用于存储 SHA-256 哈希算法的状态
    SHA256_Init(&sha256);//初始化 SHA-256 哈希算法的状态
    SHA256_Update(&sha256, str.c_str(), str.size());//更新 SHA-256 哈希算法的状态
    SHA256_Final(hash, &sha256);//计算最终的哈希值
    
    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        //将哈希值转换为十六进制字符串
        //将哈希结果（每个字节）转换为两位十六进制字符串，并拼接到 ss 里；
        //std::hex 表示使用十六进制格式输出
        //std::setw(2) 表示每个字节输出两位
        //std::setfill('0') 表示如果不足两位，前面补零
        //static_cast<int>(hash[i]) 将字节转换为整数
    }
    return ss.str();
}

std::string Block::calculateHash() const {
    std::stringstream ss;
    ss << index_ << timestamp_ << merkleRoot_ << previousHash_ << nonce_;
    //将区块的各个部分（index、timestamp、data、previousHash、nonce）转换为字符串，并拼接到 ss 里；
    return sha256(ss.str());
}


// 不断尝试不同的 nonce，直到当前计算出的哈希 hash 的前 difficulty 位是 "0000"；
// 这就模拟了"挖矿"的过程（寻找满足条件的哈希）。
void Block::mineBlock(int difficulty) {
    std::cout << index_ << " Block mined: " << merkleRoot_ << std::endl;

    std::string target(difficulty, '0');
    while (hash_.substr(0, difficulty) != target) {
        nonce_++;
        hash_ = calculateHash();
    }
    std::cout << "Block mined: " << hash_ << std::endl;
}

// 验证当前区块的哈希是否与计算出的哈希一致
bool Block::isValid() const {
    return hash_ == calculateHash();
}

std::string Block::toJson() const {
    json j;
    j["index"] = index_;
    j["timestamp"] = timestamp_;
    j["previousHash"] = previousHash_;
    j["hash"] = hash_;
    j["nonce"] = nonce_;
    j["merkleRoot"] = merkleRoot_;
    
    // 添加交易
    json transactions = json::array();
    for (const auto& tx : transactions_) {
        transactions.push_back(json::parse(tx.toJson()));
    }
    j["transactions"] = transactions;
    
    // 添加余额变更
    json balanceChanges = json::object();
    for (const auto& [key, value] : balanceChanges_) {
        balanceChanges[key] = value;
    }
    j["balanceChanges"] = balanceChanges;
    
    return j.dump();
}
