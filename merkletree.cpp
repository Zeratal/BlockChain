#include "merkletree.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>

// Helper function for calculating SHA256 hash
static std::string calculateHash(const std::string& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data.c_str(), data.size());
    SHA256_Final(hash, &sha256);
    
    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

MerkleNode::MerkleNode(const std::string& hash, const std::string& name)
    : hash_(hash)
    , name_(name)
    , left_(nullptr)
    , right_(nullptr)
    , level_(0)
{}

MerkleNode::MerkleNode(std::shared_ptr<MerkleNode> left, std::shared_ptr<MerkleNode> right)
    : left_(left)
    , right_(right)
    , level_(std::max(left->getLevel(), right ? right->getLevel() : 0) + 1)
    , name_(left->getName() + (right ? right->getName() : left->getName()))
{
    std::string combined = left->getHash() + (right ? right->getHash() : left->getHash());
    hash_ = calculateHash(combined);
    std::cout << "  MerkleNode::MerkleNode " << name_ << " " << hash_ << std::endl;
}

MerkleTree::MerkleTree(const std::vector<Transaction>& transactions) {
    std::cout << "MerkleTree::MerkleTree " << std::endl;
    if (transactions.empty()) {
        root_ = nullptr;
        return;
    }

    // Create leaf nodes from transactions
    std::vector<std::shared_ptr<MerkleNode>> nodes;
    char ch = 'A';
    for (const auto& tx : transactions) {
        std::string txHash = tx.getTransactionId();
        auto leafNode = std::make_shared<MerkleNode>(txHash, std::string(1, ch));
        ch++;
        leafNode->setLevel(0);
        leafNodes_[txHash] = leafNode;
        nodes.push_back(leafNode);
    }

    // Build the tree
    root_ = buildTree(nodes);
    
    // Build proof paths for each transaction
    buildProofPaths(root_);
}

void MerkleNode::setLevel(int level) { 
    level_ = level; 
    // std::cout << "  MerkleNode::setLevel " << name_ << " " << level_ << std::endl;
}

std::string MerkleTree::getRootHash() const {
    return root_ ? root_->getHash() : "";
}

bool MerkleTree::verifyTransaction(const Transaction& transaction) const {
    if (!root_) return false;
    
    std::string txHash = transaction.getTransactionId();
    auto it = proofPaths_.find(txHash);
    if (it == proofPaths_.end()) {
        return false;
    }
    
    return verifyPath(txHash, it->second);
}

void MerkleTree::buildProofPaths(std::shared_ptr<MerkleNode> node, const std::vector<std::pair<std::string, bool>>& path, int level) {
    if (!node) return;
    std::string indent(level * 2, ' ');

    if (node->isLeaf()) {
        proofPaths_[node->getHash()] = path;
        std::cout << indent << "buildProofPaths leaf " << node->getName() << " " << node->getHash() << std::endl;
        for (const auto& [siblingHash, isLeft] : path) {
            std::cout << indent << "  siblingHash " << siblingHash << " " << isLeft << std::endl;
        }
        return;
    }
    
    // 为左子节点构建路径
    std::vector<std::pair<std::string, bool>> leftPath = path;
    if (node->getRight()) {
        leftPath.push_back({node->getRight()->getHash(), true});
        std::cout << indent << "buildProofPaths leftPath " << node->getRight()->getName() << " " << "true" << std::endl;
    }
    buildProofPaths(node->getLeft(), leftPath, level + 1);
    
    // 为右子节点构建路径
    std::vector<std::pair<std::string, bool>> rightPath = path;
    if (node->getLeft()) {
        rightPath.push_back({node->getLeft()->getHash(), false});
        std::cout << indent << "buildProofPaths rightPath " << node->getLeft()->getName() << " " << "false" << std::endl;
    }
    buildProofPaths(node->getRight(), rightPath, level + 1);
}

bool MerkleTree::verifyPath(const std::string& txHash, const std::vector<std::pair<std::string, bool>>& path) const {
    std::string currentHash = txHash;
    
    for (auto it = path.rbegin(); it != path.rend(); ++it) {
        if (it->second) {
            // 兄弟节点在左边，当前哈希在右边
            currentHash = calculateHash(currentHash + it->first);
        } else {
            // 兄弟节点在右边，当前哈希在左边
            currentHash = calculateHash(it->first + currentHash);
        }
    }
    
    return currentHash == root_->getHash();
}

void MerkleTree::printTree() const {
    std::cout << "MerkleTree::printTree " << std::endl;
    if (!root_) {
        std::cout << "  Empty tree" << std::endl;
        return;
    }
    printNode(root_);
}

void MerkleTree::printNode(std::shared_ptr<MerkleNode> node, int level) const {
    if (!node) return;
    
    std::string indent(level * 2, ' ');
    std::cout << indent << node->getName() << "  Level " << node->getLevel() << ": " << node->getHash() << std::endl;
    
    if (node->getLeft()) {
        printNode(node->getLeft(), level + 1);
    }
    if (node->getRight()) {
        printNode(node->getRight(), level + 1);
    }
}

std::shared_ptr<MerkleNode> MerkleTree::buildTree(const std::vector<std::shared_ptr<MerkleNode>>& nodes, int level) {
    std::cout << "MerkleTree::buildTree " << std::endl;
    if (nodes.empty()) return nullptr;
    if (nodes.size() == 1) {
        // nodes[0]->setLevel(level);
        return nodes[0];
    }

    std::vector<std::shared_ptr<MerkleNode>> newNodes;
    for (size_t i = 0; i < nodes.size(); i += 2) {
        auto left = nodes[i];
        auto right = (i + 1 < nodes.size()) ? nodes[i + 1] : left;
        auto parent = std::make_shared<MerkleNode>(left, right);
        parent->setLevel(level + 1);
        newNodes.push_back(parent);
    }

    return buildTree(newNodes, level + 1);
}

std::string MerkleTree::calculateHash(const std::string& data) const {
    return ::calculateHash(data);
} 