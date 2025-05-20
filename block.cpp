#include "block.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <openssl/sha.h>

// Block 类实现
Block::Block(int index, const std::vector<Transaction>& transactions, const std::string& previousHash)
    : index_(index)
    , transactions_(transactions)
    , previousHash_(previousHash)
    , nonce_(0)
{
    std::cout << "Block::Block create" << index_ << std::endl;
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    timestamp_ = std::ctime(&now_c);
    timestamp_.pop_back(); // 移除换行符

    // Create Merkle tree and get root hash
    MerkleTree merkleTree(transactions_);
    merkleRoot_ = merkleTree.getRootHash();
    
    hash_ = calculateHash();
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
