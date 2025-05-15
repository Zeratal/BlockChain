#pragma once

#include <string>
#include <vector>
#include <memory>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <openssl/evp.h>

class Wallet {
public:
    Wallet();
    ~Wallet();

    // 生成新的密钥对
    void generateKeyPair();
    
    // 获取公钥（用于验证签名）
    std::string getPublicKey() const;
    
    // 获取私钥（用于签名）
    std::string getPrivateKey() const;
    
    // 签名数据
    std::string sign(const std::string& data) const;
    
    // 验证签名
    static bool verify(const std::string& data, 
                      const std::string& signature, 
                      const std::string& publicKey);

    static std::string sign(const std::string& data, const std::string& privateKey);

private:
    EC_KEY* keyPair_;
    
    // 将密钥转换为字符串
    static std::string keyToHex(const BIGNUM* key);
    
    // 从字符串转换为密钥
    static BIGNUM* hexToKey(const std::string& hex);
}; 