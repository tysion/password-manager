#pragma once

#include <string>
#include <vector>

namespace crypto {

/// @brief Generates a random master key.
///
/// The master key is used for encryption and hashing operations.
///
/// @return A randomly generated master key as a string.
std::string GenerateMasterKey();

/// @brief Generates a random salt.
///
/// Salt is used to add randomness to hashing operations.
///
/// @param size The size of the salt in bytes (default is 16 bytes).
/// @return A randomly generated salt as a string.
std::string GenerateSalt(size_t size = 16);

/// @brief Hashes the master key with a salt.
///
/// Uses a cryptographically secure hashing algorithm to hash the key with the
/// provided salt.
///
/// @param master_key The master key to hash.
/// @param salt The salt to use for hashing.
/// @return The resulting hash as a string.
std::string HashMasterKeyWithSalt(const std::string& master_key, const std::string& salt);

/// @brief Verifies the hash of a master key with a salt.
///
/// Compares the provided master key and salt against the given hash to verify
/// correctness.
///
/// @param master_key The master key to verify.
/// @param salt The salt used during hashing.
/// @param hash The hash to verify against.
/// @return true if the hash is valid; false otherwise.
bool VerifyMasterKeyHashWithSalt(const std::string& master_key, const std::string& salt, const std::string& hash);

/// @brief Encrypts plaintext using AES-GCM.
///
/// Encrypts the given plaintext using the provided master key. The resulting
/// data includes the IV and authentication tag.
///
/// @param plaintext The data to encrypt.
/// @param master_key The master key used for encryption.
/// @return A vector of bytes containing the encrypted data.
std::string Encrypt(const std::string& plaintext, const std::string& master_key);

/// @brief Decrypts ciphertext using AES-GCM.
///
/// Decrypts the provided data using the master key and validates its integrity
/// using the authentication tag.
///
/// @param packed_data The encrypted data, including IV and tag.
/// @param master_key The master key used for decryption.
/// @return The decrypted plaintext as a string.
std::string Decrypt(const std::string& packed_data, const std::string& master_key);

/// @brief Generates random bytes.
///
/// This function is used internally for generating keys, IVs, and other
/// cryptographic values.
///
/// @param size The number of bytes to generate.
/// @return A vector containing the random bytes.
std::vector<std::uint8_t> GenerateRandomBytes(size_t size);

}  // namespace crypto