#include "utxo.h"
#include <algorithm>
#include "nlohmann/json.hpp"

using json = nlohmann::json;
UTXO::UTXO(const std::string& txId, int outputIndex, double amount, const std::string& owner)
    : txId_(txId)
    , outputIndex_(outputIndex)
    , amount_(amount)
    , owner_(owner)
    , spent_(false)
{
    std::cout << "      UTXO::UTXO create" << "txId: " << txId_ << ", outputIndex: " << outputIndex_ << ", amount: " << amount_ << ", owner: " << owner_ << ", spent: " << spent_ << std::endl;
}

UTXO::UTXO(const json& data) {
    txId_ = data["txId"];
    outputIndex_ = data["outputIndex"];
    amount_ = data["amount"];
    owner_ = data["owner"];
    spent_ = data["spent"];
}

UTXOPool::UTXOPool() {
}

void UTXOPool::addUTXO(const UTXO& utxo) {
    std::cout << "      UTXOPool::addUTXO: " << utxo.getTxId() << ", " << utxo.getOutputIndex() << std::endl;
    utxos_[utxo.getTxId()][utxo.getOutputIndex()] = utxo;
}

void UTXOPool::removeUTXO(const std::string& txId, int outputIndex) {
    std::cout << "      removeUTXO: " << txId << ", " << outputIndex << std::endl;
    auto txIt = utxos_.find(txId);
    if (txIt != utxos_.end()) {
        std::cout << "      removeUTXO: " << txId << ", " << outputIndex << " found" << std::endl;
        txIt->second.erase(outputIndex);
        if (txIt->second.empty()) {
            std::cout << "      removeUTXO: " << txId << ", " << outputIndex << " erased" << std::endl;
            utxos_.erase(txIt);
        }
    }
}

std::vector<UTXO> UTXOPool::getUTXOsForAddress(const std::string& address) const {
    std::vector<UTXO> result;
    for (const auto& txPair : utxos_) {
        for (const auto& utxoPair : txPair.second) {
            const UTXO& utxo = utxoPair.second;
            if (utxo.getOwner() == address && !utxo.isSpent()) {
                result.push_back(utxo);
            }
        }
    }
    return result;
}

double UTXOPool::getBalance(const std::string& address) const {
    double balance = 0.0;
    for (const auto& txPair : utxos_) {
        for (const auto& utxoPair : txPair.second) {
            const UTXO& utxo = utxoPair.second;
            if (utxo.getOwner() == address && !utxo.isSpent()) {
                balance += utxo.getAmount();
            }
        }
    }
    return balance;
}

bool UTXOPool::hasEnoughFunds(const std::string& address, double amount) const {
    return getBalance(address) >= amount;
}

std::vector<UTXO> UTXOPool::selectUTXOs(const std::string& address, double amount) const {
    std::vector<UTXO> selectedUTXOs;
    double remainingAmount = amount;
    
    // 获取该地址的所有UTXO
    auto utxos = getUTXOsForAddress(address);
    
    // 按金额从大到小排序
    std::sort(utxos.begin(), utxos.end(), 
        [](const UTXO& a, const UTXO& b) { return a.getAmount() > b.getAmount(); });
    
    // 贪心算法选择UTXO
    for (const auto& utxo : utxos) {
        if (remainingAmount <= 0) break;
        
        selectedUTXOs.push_back(utxo);
        remainingAmount -= utxo.getAmount();
    }
    
    // 如果没有足够的UTXO，返回空列表
    if (remainingAmount > 0) {
        return std::vector<UTXO>();
    }
    
    return selectedUTXOs;
}

std::string UTXO::toJson() const {
    json j;
    j["txId"] = txId_;
    j["outputIndex"] = outputIndex_;
    j["amount"] = amount_;
    j["owner"] = owner_;
    j["spent"] = spent_;
    return j.dump();
} 