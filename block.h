#pragma once

#include <string>
#include <ctime>
#include <vector>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <memory>

class Block {
public:
    Block(int index, const std::string& data, const std::string& previousHash);
    
    // Getters
    int getIndex() const { return index_; }
    std::string getTimestamp() const { return timestamp_; }
    std::string getData() const { return data_; }
    std::string getPreviousHash() const { return previousHash_; }
    std::string getHash() const { return hash_; }
    int getNonce() const { return nonce_; }

    void mineBlock(int difficulty);

  bool isValid() const;
private:
    int index_;
    std::string timestamp_;
    std::string previousHash_;
    std::string hash_;
    int nonce_;
    std::string data_;
    
    std::string calculateHash() const;
    static std::string sha256(const std::string& str);
}; 