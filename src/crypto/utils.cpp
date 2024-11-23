#include "utils.hpp"

#include <cryptopp/aes.h>
#include <cryptopp/filters.h>
#include <cryptopp/gcm.h>
#include <cryptopp/hex.h>
#include <cryptopp/osrng.h>
#include <userver/crypto/hash.hpp>
#include <userver/crypto/random.hpp>

#include <stdexcept>

namespace crypto {

static constexpr size_t kAesKeySize = 32;  // 256 bits
static constexpr size_t kAesIvSize = 12;   // recommended IV size fore AES-GCM

std::vector<std::uint8_t> GenerateRandomBytes(size_t size) {
    std::vector<std::uint8_t> buffer(size);
    userver::utils::span block(buffer.data(), buffer.size());
    userver::crypto::GenerateRandomBlock(block);
    return buffer;
}

std::string GenerateMasterKey() {
    auto bytes = GenerateRandomBytes(kAesKeySize);
    return std::string(bytes.begin(), bytes.end());
}

std::string GenerateSalt(size_t size) {
    auto bytes = GenerateRandomBytes(size);
    return std::string(bytes.begin(), bytes.end());
}

std::string HashMasterKeyWithSalt(const std::string& master_key, const std::string& salt) {
    const auto combined = master_key + salt;
    const auto encoding = userver::crypto::hash::OutputEncoding::kHex;
    return userver::crypto::hash::Sha256(combined, encoding);
}

bool VerifyMasterKeyHashWithSalt(const std::string& master_key, const std::string& salt, const std::string& hash) {
    return HashMasterKeyWithSalt(master_key, salt) == hash;
}

/// Encrypts plaintext using AES-GCM.
std::string Encrypt(const std::string& plaintext, const std::string& master_key) {
    if (master_key.size() != kAesKeySize) {
        throw std::invalid_argument("Master key size must be 32 bytes (AES-256 key size).");
    }

    // generate a random IV
    const auto iv = GenerateRandomBytes(kAesIvSize);

    try {
        // prepare encryption
        CryptoPP::GCM<CryptoPP::AES>::Encryption encryptor;
        encryptor.SetKeyWithIV(
            reinterpret_cast<const std::uint8_t*>(master_key.data()), master_key.size(), iv.data(), iv.size()
        );

        // Buffers for ciphertext and tag
        std::string ciphertext;
        auto encryption_filter =
            std::make_unique<CryptoPP::AuthenticatedEncryptionFilter>(encryptor, new CryptoPP::StringSink(ciphertext));
        CryptoPP::StringSource encryptor_source(
            plaintext,
            true,
            new CryptoPP::AuthenticatedEncryptionFilter(encryptor, new CryptoPP::StringSink(ciphertext))
        );

        // combine IV and ciphertext
        std::vector<std::uint8_t> packed_data(iv.begin(), iv.end());
        packed_data.insert(packed_data.end(), ciphertext.begin(), ciphertext.end());

        return {packed_data.begin(), packed_data.end()};
    } catch (const std::exception& ex) {
        throw std::runtime_error(ex.what());
    }
}

/// Decrypts ciphertext using AES-GCM.
std::string Decrypt(const std::string& packed_data, const std::string& master_key) {
    if (master_key.size() != kAesKeySize) {
        throw std::invalid_argument("Master key size must be 32 bytes (AES-256 key size).");
    }

    if (packed_data.size() < kAesIvSize) {
        throw std::invalid_argument("Packed data size is invalid. It must include IV and tag.");
    }

    // extract IV and ciphertext
    const auto* iv = reinterpret_cast<const std::uint8_t*>(packed_data.data());
    const auto* ciphertext = reinterpret_cast<const std::uint8_t*>(packed_data.data()) + kAesIvSize;
    const size_t ciphertext_size = packed_data.size() - kAesIvSize;

    try {
        CryptoPP::GCM<CryptoPP::AES>::Decryption decryptor;
        decryptor.SetKeyWithIV(
            reinterpret_cast<const std::uint8_t*>(master_key.data()), master_key.size(), iv, kAesIvSize
        );

        std::string plaintext;

        CryptoPP::StringSource decryptor_source(
            ciphertext,
            ciphertext_size,
            true,
            new CryptoPP::AuthenticatedDecryptionFilter(decryptor, new CryptoPP::StringSink(plaintext))
        );

        return plaintext;
    } catch (const std::exception& ex) {
        throw std::runtime_error(ex.what());
    }
}

}  // namespace crypto