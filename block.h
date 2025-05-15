#pragma once

#include <string>
#include <vector>
#include <memory>
#include "transaction.h"
#include "merkletree.h"

class Block {
public:
    Block(int index, const std::vector<Transaction>& transactions, const std::string& previousHash);
    
    std::string calculateHash() const;
    void mineBlock(int difficulty);
    bool isValid() const;
    
    // Getters
    int getIndex() const { return index_; }
    std::string getTimestamp() const { return timestamp_; }
    std::string getPreviousHash() const { return previousHash_; }
    std::string getHash() const { return hash_; }
    int getNonce() const { return nonce_; }
    std::string getMerkleRoot() const { return merkleRoot_; }
    const std::vector<Transaction>& getTransactions() const { return transactions_; }

private:
    int index_;
    std::string timestamp_;
    std::vector<Transaction> transactions_;
    std::string previousHash_;
    std::string hash_;
    int nonce_;
    std::string merkleRoot_;

    static std::string sha256(const std::string& str);
}; 