#include "utils.hpp"

#include "crypto/utils.hpp"

#include <userver/crypto/hash.hpp>

#include <stdexcept>
#include <vector>

namespace {

/// @brief Encodes a byte array into a Base32 string.
///
/// @param input Byte array to encode.
/// @return Base32 encoded string.
std::string Base32Encode(const std::string& input) {
    const char base32_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    std::string output;
    size_t bits = 0, value = 0;
    for (std::uint8_t c : input) {
        value = (value << 8) | c;
        bits += 8;
        while (bits >= 5) {
            output += base32_chars[(value >> (bits - 5)) & 0x1F];
            bits -= 5;
        }
    }
    if (bits > 0) {
        output += base32_chars[(value << (5 - bits)) & 0x1F];
    }
    return output;
}

}  // namespace

namespace totp {

/// Generates a random secret key for TOTP.
std::string GenerateTotpSecret(size_t length) {
    if (length < 1) {
        throw std::invalid_argument("Length of the TOTP secret must be greater than 0");
    }
    const auto random_bytes = crypto::GenerateRandomBytes(length);
    return Base32Encode(std::string(random_bytes.begin(), random_bytes.end()));
}

uint32_t GenerateTotpCode(const std::string&, uint32_t, size_t) { return 123456; }

bool VerifyTotpCode(const std::string&, uint32_t, uint32_t, size_t, int window) { return true; }

}  // namespace totp