#include "blockchain.h"
#include <iostream>
#include <memory>
#include <vector>
#include <map>

// Blockchain 类实现
Blockchain::Blockchain(int difficulty, const std::map<std::string, double>& initialBalances)
    : difficulty_(difficulty)
{
	std::cout << "Blockchain::Blockchain createGenesisBlock" << std::endl;

    auto genesisBlock = createGenesisBlock();
    genesisBlock->setBalanceChanges(initialBalances);
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
    
    // 计算并记录余额变更
    std::map<std::string, double> balanceChanges;
    for (const auto& tx : transactions) {
        balanceChanges[tx.getFrom()] -= tx.getAmount();
        balanceChanges[tx.getTo()] += tx.getAmount();
    }
    newBlock->setBalanceChanges(balanceChanges);
    
    // 挖矿
    newBlock->mineBlock(difficulty_);
    
    // 添加区块
    chain_.push_back(newBlock);
    std::cout << "" << std::endl;
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

// 获取地址余额
double Blockchain::getBalance(const std::string& address) const {
    double balance = 0.0;
    // 遍历所有区块，计算余额
    for (const auto& block : chain_) {
        auto it = block->getBalanceChanges().find(address);
        if (it != block->getBalanceChanges().end()) {
            balance += it->second;
        }
    }
    return balance;
}

// 验证交易（包括余额检查）
bool Blockchain::validateTransaction(const Transaction& tx) const {
    // 检查签名
    if (!tx.verifySignature()) {
        std::cout << "Transaction signature verification failed" << std::endl;
        return false;
    }
    
    // 检查余额
    double balance = getBalance(tx.getFrom());
    std::cout << "Balance: " << balance << std::endl;
    return tx.hasEnoughBalance(balance);
}
