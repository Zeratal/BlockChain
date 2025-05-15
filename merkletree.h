#pragma once

#include "transaction.h"
#include <string>
#include <vector>
#include <memory>
#include <map>

class MerkleNode {
public:
    MerkleNode(const std::string& hash, const std::string& name);
    MerkleNode(std::shared_ptr<MerkleNode> left, std::shared_ptr<MerkleNode> right);
    
    const std::string& getHash() const { return hash_; }
    std::shared_ptr<MerkleNode> getLeft() const { return left_; }
    std::shared_ptr<MerkleNode> getRight() const { return right_; }
    int getLevel() const { return level_; }
    void setLevel(int level);
    bool isLeaf() const { return !left_ && !right_; }
    const std::string& getName() const { return name_; }
private:
    std::string hash_;
    std::shared_ptr<MerkleNode> left_;
    std::shared_ptr<MerkleNode> right_;
    int level_;
    std::string name_;
};

class MerkleTree {
public:
    MerkleTree(const std::vector<Transaction>& transactions);
    
    std::string getRootHash() const;
    bool verifyTransaction(const Transaction& transaction) const;
    void printTree() const;

private:
    std::shared_ptr<MerkleNode> root_;
    std::map<std::string, std::shared_ptr<MerkleNode>> leafNodes_;
    std::map<std::string, std::vector<std::pair<std::string, bool>>> proofPaths_;
    
    void buildProofPaths(std::shared_ptr<MerkleNode> node, 
                        const std::vector<std::pair<std::string, bool>>& path = {}, int level = 0);
    bool verifyPath(const std::string& txHash, 
                   const std::vector<std::pair<std::string, bool>>& path) const;
    std::shared_ptr<MerkleNode> buildTree(const std::vector<std::shared_ptr<MerkleNode>>& nodes, 
                                        int level = 0);
    void printNode(std::shared_ptr<MerkleNode> node, int level = 0) const;
    std::string calculateHash(const std::string& data) const;
}; 