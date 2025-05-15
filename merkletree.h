#pragma once
#include <string>
#include <vector>
#include <memory>
#include "transaction.h"

class MerkleNode {
public:
    MerkleNode(const std::string& hash);
    MerkleNode(std::shared_ptr<MerkleNode> left, std::shared_ptr<MerkleNode> right);
    
    std::string getHash() const { return hash_; }
    std::shared_ptr<MerkleNode> getLeft() const { return left_; }
    std::shared_ptr<MerkleNode> getRight() const { return right_; }

private:
    std::string hash_;
    std::shared_ptr<MerkleNode> left_;
    std::shared_ptr<MerkleNode> right_;
};

class MerkleTree {
public:
    MerkleTree(const std::vector<Transaction>& transactions);
    std::string getRootHash() const;
    bool verifyTransaction(const Transaction& transaction) const;

private:
    std::shared_ptr<MerkleNode> root_;
    std::vector<std::shared_ptr<MerkleNode>> leaves_;
    
    std::string calculateHash(const std::string& data) const;
    std::shared_ptr<MerkleNode> buildTree(const std::vector<std::string>& hashes);
}; 