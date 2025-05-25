#pragma once

#include "transaction.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class UTXO {
public:
    UTXO() : outputIndex_(0), amount_(0.0), spent_(false) {}
    UTXO(const std::string& txId, int outputIndex, double amount, const std::string& owner);
    UTXO(const json& data);
    
    const std::string& getTxId() const { return txId_; }
    int getOutputIndex() const { return outputIndex_; }
    double getAmount() const { return amount_; }
    const std::string& getOwner() const { return owner_; }
    bool isSpent() const { return spent_; }
    void markAsSpent() { spent_ = true; }
    
    // 添加toJson方法
    std::string toJson() const;
    
private:
    std::string txId_;
    int outputIndex_;
    double amount_;
    std::string owner_;
    bool spent_;
};

class UTXOPool {
public:
    UTXOPool();
    
    void addUTXO(const UTXO& utxo);
    void removeUTXO(const std::string& txId, int outputIndex);
    std::vector<UTXO> getUTXOsForAddress(const std::string& address) const;
    double getBalance(const std::string& address) const;
    bool hasEnoughFunds(const std::string& address, double amount) const;
    std::vector<UTXO> selectUTXOs(const std::string& address, double amount) const;
    
private:
    std::map<std::string, std::map<int, UTXO>> utxos_; // txId -> (outputIndex -> UTXO)
}; 