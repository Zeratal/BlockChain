#include "merkletree.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <algorithm>

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

MerkleNode::MerkleNode(const std::string& hash)
    : hash_(hash)
    , left_(nullptr)
    , right_(nullptr)
{}

MerkleNode::MerkleNode(std::shared_ptr<MerkleNode> left, std::shared_ptr<MerkleNode> right)
    : left_(left)
    , right_(right)
{
    std::string combined = left->getHash() + (right ? right->getHash() : left->getHash());
    hash_ = calculateHash(combined);
}
//拼接所有交易id，生成一个根节点 
MerkleTree::MerkleTree(const std::vector<Transaction>& transactions) {
    if (transactions.empty()) {
        root_ = nullptr;
        return;
    }

    // Create leaf nodes from transactions
    std::vector<std::string> hashes;
    for (const auto& tx : transactions) {
        hashes.push_back(tx.getTransactionId());
    }

    // Build the tree
    root_ = buildTree(hashes);
}

std::string MerkleTree::getRootHash() const {
    return root_ ? root_->getHash() : "";
}

bool MerkleTree::verifyTransaction(const Transaction& transaction) const {
    if (!root_) return false;
    
    std::string txHash = transaction.getTransactionId();
    // TODO: Implement transaction verification by checking the path to root
    return true;
}

std::string MerkleTree::calculateHash(const std::string& data) const {
    return ::calculateHash(data);
}

std::shared_ptr<MerkleNode> MerkleTree::buildTree(const std::vector<std::string>& hashes) {
    if (hashes.empty()) return nullptr;
    if (hashes.size() == 1) {
        return std::make_shared<MerkleNode>(hashes[0]);
    }

    std::vector<std::string> newHashes;
    for (size_t i = 0; i < hashes.size(); i += 2) {
        std::string combined = hashes[i];
        if (i + 1 < hashes.size()) {
            combined += hashes[i + 1];
        } else {
            combined += hashes[i]; // Duplicate the last hash if odd number
        }
        newHashes.push_back(calculateHash(combined));
    }

    return buildTree(newHashes);
} 