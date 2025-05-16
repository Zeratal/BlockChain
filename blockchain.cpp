#include "blockchain.h"
#include <iostream>
#include <memory>
#include <vector>
#include <map>

// Blockchain 类实现
Blockchain::Blockchain(int difficulty)
    : difficulty_(difficulty)
{
	std::cout << "Blockchain::Blockchain createGenesisBlock" << std::endl;

    auto genesisBlock = createGenesisBlock();
    chain_.push_back(genesisBlock);
}

std::shared_ptr<Block> Blockchain::createGenesisBlock() {
    std::vector<Transaction> genesisTransactions;
    return std::make_shared<Block>(0, genesisTransactions, "0");
}

void Blockchain::addBlock(const std::vector<Transaction>& transactions) {
    std::cout << "\nBlockchain::Add Block: " << transactions.size() << " transactions" << std::endl;
    // 验证所有交易
    for (const auto& tx : transactions) {
        if (!validateTransaction(tx)) {
            throw std::runtime_error("Invalid transaction: insufficient balance or invalid signature");
        }
    }
    
    // 创建新区块
    std::string previousHash = chain_.empty() ? "0" : chain_.back()->getHash();
    auto newBlock = std::make_shared<Block>(chain_.size(), transactions, previousHash);
    
    // 更新余额缓存
    for (const auto& tx : transactions) {
        balanceCache_[tx.getFrom()] -= tx.getAmount();
        balanceCache_[tx.getTo()] += tx.getAmount();
    }

    // 挖矿
    newBlock->mineBlock(difficulty_);
    
    // 添加区块
    chain_.push_back(newBlock);
    std::cout << "" << std::endl;

    // 通知钱包更新余额
    for (const auto& tx : transactions) {
        auto fromWallet = getWalletByPublicKey(tx.getFrom());
        if (fromWallet) {
            std::cout << "fromWallet: " << fromWallet->getPublicKey() << std::endl;
            fromWallet->processTransaction(tx);
        }
        auto toWallet = getWalletByPublicKey(tx.getTo());
        if (toWallet) {
            std::cout << "toWallet: " << toWallet->getPublicKey() << std::endl;
            toWallet->processTransaction(tx);
        }
    }
}

bool Blockchain::isChainValid() const {
    for (size_t i = 1; i < chain_.size(); i++) {
        const auto& currentBlock = chain_[i];
        const auto& previousBlock = chain_[i-1];
        
        if (!currentBlock->isValid()) {
            return false;
        }
        
        if (currentBlock->getPreviousHash() != previousBlock->getHash()) {
            return false;
        }
    }
    return true;
}

// 获取地址余额（使用缓存）
double Blockchain::getBalance(const std::string& address) const {
    auto it = balanceCache_.find(address);
    if (it != balanceCache_.end()) {
        return it->second;
    }
    return 0.0;
}

// 验证交易（包括余额检查）
bool Blockchain::validateTransaction(const Transaction& tx) const {
    // 检查签名
    if (!tx.verifySignature()) {
        std::cout << "Transaction signature verification failed" << std::endl;
        return false;
    }
    
    // 系统交易跳过余额检查
    if (tx.getFrom() == "SYSTEM") {
        return true;
    }
    
    // 检查余额
    double balance = getBalance(tx.getFrom());
    std::cout << "Balance: " << balance << std::endl;
    return tx.hasEnoughBalance(balance);
}

void Blockchain::registerWallet(std::shared_ptr<Wallet> wallet) {
    if (wallet) {
        wallets_[wallet->getPublicKey()] = wallet;
    }
}

std::shared_ptr<Wallet> Blockchain::getWalletByPublicKey(const std::string& publicKey) const {
    auto it = wallets_.find(publicKey);
    if (it != wallets_.end()) {
        return it->second;
    }
    return nullptr;
}
