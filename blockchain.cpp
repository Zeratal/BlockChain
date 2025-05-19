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
    std::cout << "addBlock: " << transactions.size() << std::endl;
    // 从交易池中获取待处理交易
    auto pendingTransactions = transactionPool_.getTransactions();
    // 合并新交易和待处理交易
    std::vector<Transaction> allTransactions = transactions;
    std::cout << "  allTransactions: " << allTransactions.size() << " + pendingTransactions: " << pendingTransactions.size() << std::endl;
    allTransactions.insert(allTransactions.end(), pendingTransactions.begin(), pendingTransactions.end());
    std::cout << "  allTransactions: " << allTransactions.size() << std::endl;
    // 创建新区块
    auto newBlock = std::make_shared<Block>(
        chain_.size(),
        allTransactions,
        chain_.back()->getHash()
    );
    
    // 挖矿
    std::cout << "  mineBlock: " << newBlock->getHash() << std::endl;
    newBlock->mineBlock(difficulty_);
    
    // 添加区块到链上 
    std::cout << "  addBlock: " << newBlock->getHash() << std::endl;
    chain_.push_back(newBlock);
    // 更新UTXO池
    std::cout << "  updateUTXOPool: " << newBlock->getHash() << std::endl;
    updateUTXOPool(*newBlock);
    
    // 清空交易池中已确认的交易
    for (const auto& tx : allTransactions) {
        std::cout << "    removeTransaction: " << tx.getTransactionId() << std::endl;
        transactionPool_.removeTransaction(tx.getTransactionId());
    }
}

bool Blockchain::addTransactionToPool(const Transaction& transaction) {
    std::cout << "addTransactionToPool: " << transaction.getTransactionId() << std::endl;
    return transactionPool_.addTransaction(transaction, utxoPool_);
}

std::vector<Transaction> Blockchain::getPendingTransactions() const {
    return transactionPool_.getTransactions();
}

void Blockchain::updateUTXOPool(const Block& block) {
    // 处理区块中的每个交易
    for (const auto& tx : block.getTransactions()) {
        // 移除已使用的UTXO
        std::cout << "updateUTXOPool: " << tx.getTransactionId() << std::endl;
        std::cout << "  inputs: " << tx.getInputs().size() << std::endl;
        for (const auto& input : tx.getInputs()) {
            utxoPool_.removeUTXO(input.getTxId(), input.getOutputIndex());
            std::cout << "  removeUTXO: " << input.getTxId() << ", " << input.getOutputIndex() << std::endl;
        }
        
        // 添加新的UTXO
        std::cout << "  outputs: " << tx.getOutputs().size() << std::endl;
        for (size_t i = 0; i < tx.getOutputs().size(); ++i) {
            std::cout << "  addUTXO: " << tx.getTransactionId() << ", " << i << std::endl;
            const auto& output = tx.getOutputs()[i];
            UTXO utxo(tx.getTransactionId(), i, output.getAmount(), output.getOwner());
            utxoPool_.addUTXO(utxo);
            std::cout << "  addUTXO: " << utxo.getTxId() << ", " << utxo.getOutputIndex() << std::endl;
        }
    }
}

double Blockchain::getBalance(const std::string& address) const {
    std::cout << "getBalance: " << address << std::endl;
    return utxoPool_.getBalance(address);
}

bool Blockchain::isChainValid() const {
    for (size_t i = 1; i < chain_.size(); ++i) {
        const auto& currentBlock = chain_[i];
        const auto& previousBlock = chain_[i - 1];
        
        // 验证当前区块的哈希
        if (currentBlock->getHash() != currentBlock->calculateHash()) {
            return false;
        }
        
        // 验证区块链接
        if (currentBlock->getPreviousHash() != previousBlock->getHash()) {
            return false;
        }
    }
    return true;
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
    wallets_[wallet->getPublicKey()] = wallet;
}

std::shared_ptr<Wallet> Blockchain::getWalletByPublicKey(const std::string& publicKey) const {
    auto it = wallets_.find(publicKey);
    if (it != wallets_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<UTXO> Blockchain::getUTXOsForAddress(const std::string& address) const {
    return utxoPool_.getUTXOsForAddress(address);
}
