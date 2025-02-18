#include <iostream>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <benchmark/benchmark.h>
#include <vector>
#include <cstring>
#include <sstream>

// #ifndef MSG_LEN 
// #define MSG_LEN 1500
// #endif // !MSG_LEN 

const std::string ip_tcp_header = "45480088c696400040064cbc83fea3b7c2fe3c219ef60016d23181a5352ad37d8018306227500000";

std::vector<uint8_t> hexStringToByteVector(const std::string &hexString) {
    std::vector<uint8_t> byteVector;
    for (size_t i = 0; i < hexString.length(); i += 2) {
        unsigned int byte;
        std::stringstream ss;
        ss << std::hex << hexString.substr(i, 2);
        ss >> byte;
        byteVector.push_back(static_cast<uint8_t>(byte));
    }
    return byteVector;
}

void encrypt(const std::vector<uint8_t>& plaintext, std::vector<uint8_t>& ciphertext, std::vector<uint8_t>& tag, std::vector<uint8_t>& key, std::vector<uint8_t>& nonce) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key.data(), nonce.data());
    int len;
    ciphertext.resize(plaintext.size());
    EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), plaintext.size());
    EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len);
    tag.resize(16);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag.data());
    EVP_CIPHER_CTX_free(ctx);
}

void decrypt(const std::vector<uint8_t>& ciphertext, std::vector<uint8_t>& plaintext, const std::vector<uint8_t>& tag, const std::vector<uint8_t>& key, const std::vector<uint8_t>& nonce) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key.data(), nonce.data());
    int len;
    plaintext.resize(ciphertext.size());
    EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), ciphertext.size());
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, 16, (void*)tag.data());
    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) <= 0) {
        std::cerr << "Decryption failed!" << std::endl;
    }
    EVP_CIPHER_CTX_free(ctx);
}

static void BM_Encrypt(benchmark::State& state) {
    // Perform setup here
    std::vector<uint8_t> key(32), nonce(12);
    std::vector<uint8_t> payload(state.range(0)-40);
    std::vector<uint8_t> ciphertext, tag;
    std::vector<uint8_t> tcp_header = hexStringToByteVector(ip_tcp_header);

    RAND_bytes(key.data(), key.size());
    RAND_bytes(nonce.data(), nonce.size());
    RAND_bytes(payload.data(), payload.size());

    std::vector<uint8_t> plaintext;
    plaintext.reserve(state.range(0));
    plaintext.insert(plaintext.end(), tcp_header.begin(), tcp_header.end());
    plaintext.insert(plaintext.end(), payload.begin(), payload.end());

    for (auto _ : state) {
        // This code gets timed
        encrypt(plaintext, ciphertext, tag, key, nonce);
    }
}

static void BM_Decrypt(benchmark::State& state) {
    std::vector<uint8_t> key(32), nonce(12);
    std::vector<uint8_t> payload(state.range(0)-40);
    std::vector<uint8_t> ciphertext, tag, decrypted;
    std::vector<uint8_t> tcp_header = hexStringToByteVector(ip_tcp_header);

    RAND_bytes(key.data(), key.size());
    RAND_bytes(nonce.data(), nonce.size());
    RAND_bytes(payload.data(), payload.size());

    std::vector<uint8_t> plaintext;
    plaintext.reserve(state.range(0));
    plaintext.insert(plaintext.end(), tcp_header.begin(), tcp_header.end());
    plaintext.insert(plaintext.end(), payload.begin(), payload.end());

    encrypt(plaintext, ciphertext, tag, key, nonce);

    for (auto _ : state) {
        decrypt(ciphertext, decrypted, tag, key, nonce);
    }
}

BENCHMARK(BM_Encrypt)
    ->Args({40})
    ->Args({52})
    ->Args({100})
    ->Args({120})
    ->Args({150})
    ->Args({500})
    ->Args({800})
    ->Args({1000})
    ->Args({1300}) 
    ->Args({1500})
    ->Args({1800})
    ->Args({2000});

BENCHMARK(BM_Decrypt)
    ->Args({40})
    ->Args({52})
    ->Args({100})
    ->Args({120})
    ->Args({150})
    ->Args({500})
    ->Args({800})
    ->Args({1000})
    ->Args({1300}) 
    ->Args({1500})
    ->Args({1800})
    ->Args({2000});

// Run the benchmark
BENCHMARK_MAIN();
