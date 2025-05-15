#pragma once
#include <string>
#include <ctime>

class Transaction {
public:
    Transaction(const std::string& from, const std::string& to, double amount);
    
    // Getters
    std::string getFrom() const { return from_; }
    std::string getTo() const { return to_; }
    double getAmount() const { return amount_; }
    std::string getTimestamp() const { return timestamp_; }
    std::string getTransactionId() const { return transactionId_; }

private:
    std::string from_;        // Sender's address
    std::string to_;          // Receiver's address
    double amount_;           // Transaction amount
    std::string timestamp_;   // Transaction timestamp
    std::string transactionId_; // Unique transaction ID
}; 