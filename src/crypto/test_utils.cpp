#include "utils.hpp"

#include <userver/utest/utest.hpp>

using namespace crypto;

// Test GenerateMasterKey
TEST(CryptoUtilsTest, GenerateMasterKey_UniqueKeys) {
    auto key1 = GenerateMasterKey();
    auto key2 = GenerateMasterKey();
    EXPECT_NE(key1, key2);
    EXPECT_EQ(key1.size(), 32);  // Assuming the key size is 32 bytes
}

// Test GenerateSalt
TEST(CryptoUtilsTest, GenerateSalt_UniqueSalts) {
    auto salt1 = GenerateSalt();
    auto salt2 = GenerateSalt();
    EXPECT_NE(salt1, salt2);
    EXPECT_EQ(salt1.size(), 16);  // Default salt size
}

// Test HashMasterKeyWithSalt and VerifyMasterKeyHashWithSalt
TEST(CryptoUtilsTest, HashAndVerifyMasterKey) {
    auto master_key = GenerateMasterKey();
    auto salt = GenerateSalt();

    auto hash = HashMasterKeyWithSalt(master_key, salt);
    EXPECT_FALSE(hash.empty());

    bool is_valid = VerifyMasterKeyHashWithSalt(master_key, salt, hash);
    EXPECT_TRUE(is_valid);

    auto invalid_key = GenerateMasterKey();
    bool is_invalid = VerifyMasterKeyHashWithSalt(invalid_key, salt, hash);
    EXPECT_FALSE(is_invalid);
}

// Test Encrypt and Decrypt
TEST(CryptoUtilsTest, EncryptDecrypt_Success) {
    auto master_key = GenerateMasterKey();
    std::string plaintext = "Sensitive data";

    auto encrypted_data = Encrypt(plaintext, master_key);
    EXPECT_FALSE(encrypted_data.empty());

    auto decrypted_data = Decrypt(encrypted_data, master_key);
    EXPECT_EQ(decrypted_data, plaintext);
}

// Test Encrypt and Decrypt with invalid key
TEST(CryptoUtilsTest, EncryptDecrypt_InvalidKey) {
    auto master_key = GenerateMasterKey();
    auto wrong_key = GenerateMasterKey();
    std::string plaintext = "Sensitive data";

    auto encrypted_data = Encrypt(plaintext, master_key);
    EXPECT_FALSE(encrypted_data.empty());

    EXPECT_THROW(Decrypt(encrypted_data, wrong_key), std::runtime_error);
}
