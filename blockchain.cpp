#include "blockchain.h"
#include <iostream>
#include <memory>
#include <vector>
#include <map>
#include <chrono>
#include <mutex>

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

    // 添加创世交易
    Transaction genesisTx(
        "SYSTEM",  // 从系统
        "GENESIS", // 到创世地址
        1000000    // 初始金额
    );
    genesisTx.setSignature("GENESIS_SIGNATURE");
    genesisTransactions.push_back(genesisTx);
    
    // 使用高精度时间戳作为唯一标识
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    std::string uniqueId = std::to_string(nanoseconds);
    
    return std::make_shared<Block>(0, genesisTransactions, uniqueId);
}

void Blockchain::addBlock(const std::vector<Transaction>& transactions, bool usePendingTxs) {
    std::vector<Transaction> blockTransactions;
    
    if (usePendingTxs) {
        // 使用链上的待处理交易
        blockTransactions = getPendingTransactions();
    } else {
        // 使用传入的交易
        blockTransactions = transactions;
    }
    
    // 创建新区块
    auto newBlock = std::make_shared<Block>(
        chain_.size(),
        blockTransactions,
        chain_.empty() ? "0" : chain_.back()->getHash()
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
    
    // 清理已处理的交易
    if (usePendingTxs) {
        clearPendingTransactions();
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
    std::cout << "\n  updateUTXOPool: " << block.getTransactions().size() << std::endl;
    for (const auto& tx : block.getTransactions()) {
        // 移除已使用的UTXO
        std::cout << "    updateUTXOPool: " << tx.getTransactionId() << std::endl;
        std::cout << "    inputs: " << tx.getInputs().size() << std::endl;
        for (const auto& input : tx.getInputs()) {
            utxoPool_.removeUTXO(input.getTxId(), input.getOutputIndex());
            std::cout << "      removeUTXO: " << input.getTxId() << ", " << input.getOutputIndex() << std::endl;
        }
        
        // 添加新的UTXO
        std::cout << "    outputs: " << tx.getOutputs().size() << std::endl;
        for (size_t i = 0; i < tx.getOutputs().size(); ++i) {
            std::cout << "      addUTXO: " << tx.getTransactionId() << ", " << i << std::endl;
            const auto& output = tx.getOutputs()[i];
            UTXO utxo(tx.getTransactionId(), i, output.getAmount(), output.getOwner());
            utxoPool_.addUTXO(utxo);
            // std::cout << "      addUTXO: UTXO: " << utxo.getTxId() << ", " << utxo.getOutputIndex() << ", " << utxo.getAmount() << ", " << utxo.getOwner() << std::endl;
        }
    }
}

void Blockchain::updateBalance(const std::string& address, double balance) {
    std::lock_guard<std::mutex> lock(balances_mutex_);
    balances_[address] = balance;
}

double Blockchain::getBalance(const std::string& address) const {
    std::lock_guard<std::mutex> lock(balances_mutex_);
    
    auto it = balances_.find(address);
    if (it != balances_.end()) {
        return it->second;
    }
    
    double balance = 0.0;
    auto utxos = getUTXOsForAddress(address);
    for (const auto& utxo : utxos) {
        balance += utxo.getAmount();
    }
    
    balances_[address] = balance;
    return balance;
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
    std::cout << "Blockchain::validateTransaction: " << tx.getTransactionId() << std::endl;
    
    // 系统交易只需要验证签名
    if (tx.getFrom() == "SYSTEM") {
        return tx.verifySignature();
    }
    
    // 普通交易需要验证签名和余额
    if (!tx.verifySignature()) {
        std::cout << "Transaction signature verification failed" << std::endl;
        return false;
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

std::vector<Block> Blockchain::getBlocksFromHeight(int startHeight) const {
    std::vector<Block> blocks;
    for (size_t i = startHeight; i < chain_.size(); ++i) {
        // 从智能指针获取 Block 对象
        const Block& block = *chain_[i];
        blocks.push_back(block);
    }
    return blocks;
}

void Blockchain::clearPendingTransactions() {
    std::cout << "clearPendingTransactions" << std::endl;
    transactionPool_.clear();
}

void Blockchain::updateUTXO(const UTXO& utxo) {
    utxos_[utxo.getOwner()].push_back(utxo);
}

void Blockchain::updateUTXOs(const std::string& address, const std::vector<UTXO>& utxos) {
    utxos_[address] = utxos;
}

std::vector<UTXO> Blockchain::getAllUTXOs() const {
    std::vector<UTXO> allUtxos;
    for (const auto& [address, utxoList] : utxos_) {
        allUtxos.insert(allUtxos.end(), utxoList.begin(), utxoList.end());
    }
    return allUtxos;
}

bool Blockchain::verifyBlock(const Block& block) const {
    // 1. 验证区块索引
    if (block.getIndex() != chain_.size()) {
        std::cout << "Invalid block index" << std::endl;
        return false;
    }
    
    // 2. 验证前一个区块的哈希
    if (!chain_.empty()) {
        if (block.getPreviousHash() != chain_.back()->getHash()) {
            std::cout << "Invalid previous hash" << std::endl;
            return false;
        }
    } else if (block.getPreviousHash() != "0") {
        std::cout << "Invalid genesis block previous hash" << std::endl;
        return false;
    }
    
    // 3. 验证区块哈希
    if (block.getHash() != block.calculateHash()) {
        std::cout << "Invalid block hash" << std::endl;
        return false;
    }
    
    // 4. 验证区块难度
    if (!block.verifyDifficulty(difficulty_)) {
        std::cout << "Block does not meet difficulty requirement" << std::endl;
        return false;
    }
    
    // 5. 验证区块中的交易
    for (const auto& tx : block.getTransactions()) {
        if (!validateTransaction(tx)) {
            std::cout << "Invalid transaction in block" << std::endl;
            return false;
        }
    }
    
    // 6. 验证Merkle树根
    std::vector<Transaction> transactions = block.getTransactions();
    MerkleTree merkleTree(transactions);
    if (block.getMerkleRoot() != merkleTree.getRootHash()) {
        std::cout << "Invalid merkle root" << std::endl;
        return false;
    }
    
    return true;
}


