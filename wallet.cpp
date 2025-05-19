#include "wallet.h"
#include "transaction.h"
#include <openssl/sha.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <sstream>
#include <iomanip>
#include <iostream>

Wallet::Wallet() : keyPair_(nullptr) {
    // 初始化 OpenSSL
    OpenSSL_add_all_algorithms();
    generateKeyPair();
}

Wallet::~Wallet() {
    if (keyPair_) {
        EC_KEY_free(keyPair_);
    }
    // 清理 OpenSSL
    EVP_cleanup();
}

void Wallet::generateKeyPair() {
    // 创建新的 EC_KEY
    keyPair_ = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!keyPair_) {
        throw std::runtime_error("Failed to create EC_KEY");
    }

    // 生成密钥对
    if (!EC_KEY_generate_key(keyPair_)) {
        EC_KEY_free(keyPair_);
        keyPair_ = nullptr;
        throw std::runtime_error("Failed to generate key pair");
    }
}

std::string Wallet::getPublicKey() const {
    if (!keyPair_) {
        throw std::runtime_error("No key pair generated");
    }

    const EC_POINT* pub = EC_KEY_get0_public_key(keyPair_);
    const EC_GROUP* group = EC_KEY_get0_group(keyPair_);
    
    BIGNUM* x = BN_new();
    BIGNUM* y = BN_new();
    
    if (!EC_POINT_get_affine_coordinates_GFp(group, pub, x, y, nullptr)) {
        BN_free(x);
        BN_free(y);
        throw std::runtime_error("Failed to get public key coordinates");
    }

    std::string result = keyToHex(x) + ":" + keyToHex(y);
    
    BN_free(x);
    BN_free(y);
    return result;
}

std::string Wallet::getPrivateKey() const {
    if (!keyPair_) {
        throw std::runtime_error("No key pair generated");
    }

    const BIGNUM* priv = EC_KEY_get0_private_key(keyPair_);
    return keyToHex(priv);
}

std::string Wallet::sign(const std::string& data) const {
    if (!keyPair_) {
        throw std::runtime_error("No key pair generated");
    }

    // 计算数据的 SHA256 哈希
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data.c_str(), data.size());
    SHA256_Final(hash, &sha256);

    // 使用私钥签名
    ECDSA_SIG* sig = ECDSA_do_sign(hash, SHA256_DIGEST_LENGTH, keyPair_);
    if (!sig) {
        throw std::runtime_error("Failed to sign data");
    }

    // 获取签名值
    const BIGNUM* r = nullptr;
    const BIGNUM* s = nullptr;
    ECDSA_SIG_get0(sig, &r, &s);

    // 将签名转换为十六进制字符串
    std::string result = keyToHex(r) + ":" + keyToHex(s);

    // 清理
    ECDSA_SIG_free(sig);
    return result;
}

std::string Wallet::sign(const std::string& data, const std::string& privateKey) {
    // 创建新的 EC_KEY
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!key) {
        throw std::runtime_error("Failed to create EC_KEY");
    }

    // 从十六进制字符串设置私钥
    BIGNUM* priv = hexToKey(privateKey);
    if (!priv) {
        EC_KEY_free(key);
        throw std::runtime_error("Failed to convert private key");
    }

    if (!EC_KEY_set_private_key(key, priv)) {
        BN_free(priv);
        EC_KEY_free(key);
        throw std::runtime_error("Failed to set private key");
    }

    // 计算数据的 SHA256 哈希
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data.c_str(), data.size());
    SHA256_Final(hash, &sha256);

    // 使用私钥签名
    ECDSA_SIG* sig = ECDSA_do_sign(hash, SHA256_DIGEST_LENGTH, key);
    if (!sig) {
        BN_free(priv);
        EC_KEY_free(key);
        throw std::runtime_error("Failed to sign data");
    }

    // 获取签名值
    const BIGNUM* r = nullptr;
    const BIGNUM* s = nullptr;
    ECDSA_SIG_get0(sig, &r, &s);

    // 将签名转换为十六进制字符串
    std::string result = keyToHex(r) + ":" + keyToHex(s);

    // 清理
    ECDSA_SIG_free(sig);
    BN_free(priv);
    EC_KEY_free(key);
    return result;
}

bool Wallet::verify(const std::string& data, 
                   const std::string& signature, 
                   const std::string& publicKey) {
    // std::cout << "Verifying signature with public key: " << publicKey << std::endl;
    // 解析公钥
    size_t pos = publicKey.find(':');
    if (pos == std::string::npos) {
        std::cout << "Invalid public key format" << std::endl;
        return false;
    }

    std::string xStr = publicKey.substr(0, pos);
    std::string yStr = publicKey.substr(pos + 1);

    // 创建 EC_KEY
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!key) {
        std::cout << "Failed to create EC_KEY" << std::endl;
        return false;
    }

    // 设置公钥
    BIGNUM* x = hexToKey(xStr);
    BIGNUM* y = hexToKey(yStr);
    if (!x || !y) {
        std::cout << "Failed to convert public key coordinates" << std::endl;
        EC_KEY_free(key);
        return false;
    }

    EC_POINT* pub = EC_POINT_new(EC_KEY_get0_group(key));
    if (!pub) {
        std::cout << "Failed to set public key coordinates" << std::endl;
        BN_free(x);
        BN_free(y);
        EC_KEY_free(key);
        return false;
    }

    if (!EC_POINT_set_affine_coordinates_GFp(EC_KEY_get0_group(key), pub, x, y, nullptr)) {
        std::cout << "Failed to set affine coordinates" << std::endl;
        EC_POINT_free(pub);
        BN_free(x);
        BN_free(y);
        EC_KEY_free(key);
        return false;
    }

    if (!EC_KEY_set_public_key(key, pub)) {
        std::cout << "Failed to set public key" << std::endl;
        EC_POINT_free(pub);
        BN_free(x);
        BN_free(y);
        EC_KEY_free(key);
        return false;
    }

    // 解析签名
    pos = signature.find(':');
    if (pos == std::string::npos) {
        std::cout << "Failed to find signature" << std::endl;
        EC_POINT_free(pub);
        BN_free(x);
        BN_free(y);
        EC_KEY_free(key);
        return false;
    }

    std::string rStr = signature.substr(0, pos);
    std::string sStr = signature.substr(pos + 1);

    BIGNUM* r = hexToKey(rStr);
    BIGNUM* s = hexToKey(sStr);
    if (!r || !s) {
        std::cout << "Failed to parse signature" << std::endl;
        EC_POINT_free(pub);
        BN_free(x);
        BN_free(y);
        EC_KEY_free(key);
        return false;
    }

    // 创建签名对象
    ECDSA_SIG* sig = ECDSA_SIG_new();
    if (!sig) {
        std::cout << "Failed to create signature" << std::endl;
        BN_free(r);
        BN_free(s);
        EC_POINT_free(pub);
        BN_free(x);
        BN_free(y);
        EC_KEY_free(key);
        return false;
    }

    ECDSA_SIG_set0(sig, r, s);

    // 计算数据的 SHA256 哈希
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data.c_str(), data.size());
    SHA256_Final(hash, &sha256);

    // 验证签名
    int result = ECDSA_do_verify(hash, SHA256_DIGEST_LENGTH, sig, key);

    // 清理
    ECDSA_SIG_free(sig);
    EC_POINT_free(pub);
    BN_free(x);
    BN_free(y);
    EC_KEY_free(key);

    return result == 1;
}

std::string Wallet::keyToHex(const BIGNUM* key) {
    char* hex = BN_bn2hex(key);
    if (!hex) {
        throw std::runtime_error("Failed to convert key to hex");
    }
    std::string result(hex);
    OPENSSL_free(hex);
    return result;
}

BIGNUM* Wallet::hexToKey(const std::string& hex) {
    BIGNUM* key = BN_new();
    if (!key) {
        return nullptr;
    }
    if (!BN_hex2bn(&key, hex.c_str())) {
        BN_free(key);
        return nullptr;
    }
    return key;
}

void Wallet::processTransaction(const Transaction& tx) {
    if (tx.getFrom() == getPublicKey()) {
        balance_ -= tx.getAmount();
    }
    if (tx.getTo() == getPublicKey()) {
        balance_ += tx.getAmount();
    }
}

// bool Wallet::deductBalance(double amount) {
//     if (balance_ >= amount) {
//         balance_ -= amount;
//         return true;
//     }
//     return false;
// }
