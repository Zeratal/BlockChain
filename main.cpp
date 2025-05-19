#include "blockchain.h"
#include "wallet.h"
#include <iostream>
#include <chrono>
#include <map>

int main() {
    try {
        // 创建钱包
        std::cout << "Creating wallets..." << std::endl;
        auto aliceWallet = std::make_shared<Wallet>();
        auto bobWallet = std::make_shared<Wallet>();
        auto charlieWallet = std::make_shared<Wallet>();
                
        // Get public keys
        std::string alicePublicKey = aliceWallet->getPublicKey();
        std::string bobPublicKey = bobWallet->getPublicKey();
        std::string charliePublicKey = charlieWallet->getPublicKey();
        
        std::cout << "Alice's public key: " << alicePublicKey << std::endl;
        std::cout << "Bob's public key: " << bobPublicKey << std::endl;
        std::cout << "Charlie's public key: " << charliePublicKey << std::endl;
        
        // Create blockchain
        Blockchain blockchain(4);
        
        // Register wallets
        blockchain.registerWallet(aliceWallet);
        blockchain.registerWallet(bobWallet);
        blockchain.registerWallet(charlieWallet);
        
        // Create genesis block transactions
        std::cout << "\nCreating genesis block transactions..." << std::endl;
        blockchain.addBlock({
            Transaction::createSystemTransaction(alicePublicKey, 100.0),
            Transaction::createSystemTransaction(bobPublicKey, 50.0),
            Transaction::createSystemTransaction(charliePublicKey, 25.0)
        });
        
        // Print initial balances
        std::cout << "\nInitial balances:" << std::endl;
        std::cout << "Alice: " << blockchain.getBalance(alicePublicKey) << std::endl;
        std::cout << "Bob: " << blockchain.getBalance(bobPublicKey) << std::endl;
        std::cout << "Charlie: " << blockchain.getBalance(charliePublicKey) << std::endl;
        
        // Create first group of transactions
        std::cout << "\nCreating first group of transactions..." << std::endl;
        
        // Alice sends 10 coins to Bob
        Transaction tx1(alicePublicKey, bobPublicKey, 10.0);
        std::cout << "tx1: " << tx1.getTransactionId() << std::endl;
        auto aliceUTXOs = blockchain.getUTXOsForAddress(alicePublicKey);
        std::cout << "aliceUTXOs: " << aliceUTXOs.size() << std::endl;
        std::string tx1Signature = aliceWallet->sign(tx1.getTransactionId());
        tx1.setSignature(tx1Signature);
        for (const auto& utxo : aliceUTXOs) {
            std::cout << "addInput utxo: " << utxo.getTxId() << ", " << utxo.getOutputIndex() << std::endl;
            tx1.addInput(TransactionInput(utxo.getTxId(), utxo.getOutputIndex(), tx1Signature));
        }
        tx1.addOutput(TransactionOutput(10.0, bobPublicKey));
        if (blockchain.getBalance(alicePublicKey) > 10.0) {
            std::cout << "addOutput alicePublicKey: " << blockchain.getBalance(alicePublicKey) - 10.0 << std::endl;
            tx1.addOutput(TransactionOutput(blockchain.getBalance(alicePublicKey) - 10.0, alicePublicKey));
        }
        
        // Bob sends 5 coins to Charlie
        Transaction tx2(bobPublicKey, charliePublicKey, 5.0);
        std::cout << "tx2: " << tx2.getTransactionId() << std::endl;
        auto bobUTXOs = blockchain.getUTXOsForAddress(bobPublicKey);
        std::cout << "bobUTXOs: " << bobUTXOs.size() << std::endl;
        std::string tx2Signature = bobWallet->sign(tx2.getTransactionId());
        tx2.setSignature(tx2Signature);
        for (const auto& utxo : bobUTXOs) {
            std::cout << "addInput utxo: " << utxo.getTxId() << ", " << utxo.getOutputIndex() << std::endl;
            tx2.addInput(TransactionInput(utxo.getTxId(), utxo.getOutputIndex(), tx2Signature));
        }
        tx2.addOutput(TransactionOutput(5.0, charliePublicKey));
        if (blockchain.getBalance(bobPublicKey) > 5.0) {
            std::cout << "addOutput bobPublicKey: " << blockchain.getBalance(bobPublicKey) - 5.0 << std::endl;
            tx2.addOutput(TransactionOutput(blockchain.getBalance(bobPublicKey) - 5.0, bobPublicKey));
        }
        
        // Charlie sends 2.5 coins to Alice
        Transaction tx3(charliePublicKey, alicePublicKey, 2.5);
        auto charlieUTXOs = blockchain.getUTXOsForAddress(charliePublicKey);
        std::cout << "charlieUTXOs: " << charlieUTXOs.size() << std::endl;
        std::string tx3Signature = charlieWallet->sign(tx3.getTransactionId());
        tx3.setSignature(tx3Signature);
        for (const auto& utxo : charlieUTXOs) {
            std::cout << "addInput utxo: " << utxo.getTxId() << ", " << utxo.getOutputIndex() << std::endl;
            tx3.addInput(TransactionInput(utxo.getTxId(), utxo.getOutputIndex(), tx3Signature));
        }
        tx3.addOutput(TransactionOutput(2.5, alicePublicKey));
        if (blockchain.getBalance(charliePublicKey) > 2.5) {
            std::cout << "addOutput charliePublicKey: " << blockchain.getBalance(charliePublicKey) - 2.5 << std::endl;
            tx3.addOutput(TransactionOutput(blockchain.getBalance(charliePublicKey) - 2.5, charliePublicKey));
        }
        
        // Add transactions to pool
        std::cout << "addTransactionToPool tx1" << std::endl;
        blockchain.addTransactionToPool(tx1);
        std::cout << "addTransactionToPool tx2" << std::endl;
        blockchain.addTransactionToPool(tx2);
        std::cout << "addTransactionToPool tx3" << std::endl;
        blockchain.addTransactionToPool(tx3);
        
        // Add block (includes transactions from pool)
        blockchain.addBlock({});
        
        // Print balances
        std::cout << "\nBalances after first block:" << std::endl;
        std::cout << "Alice: " << blockchain.getBalance(alicePublicKey) << std::endl;
        std::cout << "Bob: " << blockchain.getBalance(bobPublicKey) << std::endl;
        std::cout << "Charlie: " << blockchain.getBalance(charliePublicKey) << std::endl;
        
        // Create second group of transactions
        std::cout << "\nCreating second group of transactions..." << std::endl;
        
        // Alice sends 7.5 coins to Charlie
        Transaction tx4(alicePublicKey, charliePublicKey, 7.5);
        aliceUTXOs = blockchain.getUTXOsForAddress(alicePublicKey);
        std::string tx4Signature = aliceWallet->sign(tx4.getTransactionId());
        for (const auto& utxo : aliceUTXOs) {
            tx4.addInput(TransactionInput(utxo.getTxId(), utxo.getOutputIndex(), tx4Signature));
        }
        tx4.addOutput(TransactionOutput(7.5, charliePublicKey));
        if (blockchain.getBalance(alicePublicKey) > 7.5) {
            tx4.addOutput(TransactionOutput(blockchain.getBalance(alicePublicKey) - 7.5, alicePublicKey));
        }
        
        // Charlie sends 3 coins to Bob
        Transaction tx5(charliePublicKey, bobPublicKey, 3.0);
        charlieUTXOs = blockchain.getUTXOsForAddress(charliePublicKey);
        std::string tx5Signature = charlieWallet->sign(tx5.getTransactionId());
        for (const auto& utxo : charlieUTXOs) {
            tx5.addInput(TransactionInput(utxo.getTxId(), utxo.getOutputIndex(), tx5Signature));
        }
        tx5.addOutput(TransactionOutput(3.0, bobPublicKey));
        if (blockchain.getBalance(charliePublicKey) > 3.0) {
            tx5.addOutput(TransactionOutput(blockchain.getBalance(charliePublicKey) - 3.0, charliePublicKey));
        }
        
        // Add transactions to pool
        blockchain.addTransactionToPool(tx4);
        blockchain.addTransactionToPool(tx5);
        
        // Add block
        blockchain.addBlock({});
        
        // Print balances
        std::cout << "\nBalances after second block:" << std::endl;
        std::cout << "Alice: " << blockchain.getBalance(alicePublicKey) << std::endl;
        std::cout << "Bob: " << blockchain.getBalance(bobPublicKey) << std::endl;
        std::cout << "Charlie: " << blockchain.getBalance(charliePublicKey) << std::endl;
        
        // Verify blockchain
        std::cout << "\nIs blockchain valid: " << (blockchain.isChainValid() ? "Yes" : "No") << std::endl;
        
        // Print final balances
        std::cout << "\nFinal balances:" << std::endl;
        std::cout << "Alice: " << blockchain.getBalance(alicePublicKey) << std::endl;
        std::cout << "Bob: " << blockchain.getBalance(bobPublicKey) << std::endl;
        std::cout << "Charlie: " << blockchain.getBalance(charliePublicKey) << std::endl;
        
        // Print blockchain information
        std::cout << "\nBlockchain Information:" << blockchain.getChain().size() << std::endl;
        for (const auto& block : blockchain.getChain()) {
            std::cout << "\n  ------------" << "Index: " << block->getIndex() << "------------" << std::endl;
            std::cout << "  Timestamp: " << block->getTimestamp() << std::endl;
            std::cout << "  Merkle Root: " << block->getMerkleRoot() << std::endl;
            std::cout << "  Previous Hash: " << block->getPreviousHash() << std::endl;
            std::cout << "  Hash: " << block->getHash() << std::endl;
            std::cout << "  Nonce: " << block->getNonce() << std::endl;
            
            std::cout << "\n  Transactions:" << block->getTransactions().size() << std::endl;
            for (const auto& tx : block->getTransactions()) {
                std::cout << "    ------------------------" << std::endl;
                std::cout << "    From: " << tx.getFrom() << std::endl;
                std::cout << "    To: " << tx.getTo() << std::endl;
                std::cout << "    Amount: " << tx.getAmount() << std::endl;
                std::cout << "    Transaction ID: " << tx.getTransactionId() << std::endl;
                std::cout << "    Signature Valid: " << (tx.verifySignature() ? "Yes" : "No") << std::endl;
                
                std::cout << "    Inputs:" << tx.getInputs().size() << std::endl;
                for (const auto& input : tx.getInputs()) {
                    std::cout << "      Transaction ID: " << input.getTxId() << std::endl;
                    std::cout << "      Output Index: " << input.getOutputIndex() << std::endl;
                }
                
                std::cout << "    Outputs:" << tx.getOutputs().size() << std::endl;
                for (const auto& output : tx.getOutputs()) {
                    std::cout << "      Amount: " << output.getAmount() << std::endl;
                    std::cout << "      Owner: " << output.getOwner() << std::endl;
                }
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        getchar();
        return 1;
    }
    getchar();
    return 0;
} 