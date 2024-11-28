#include "utils.hpp"

#include "crypto/utils.hpp"

#include <userver/crypto/base64.hpp>
#include <userver/crypto/hash.hpp>

#include <array>
#include <vector>

namespace {

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

    while (output.size() % 8 != 0) {
        output += '=';
    }

    return output;
}

std::string Base32Decode(const std::string& input) {
    static const std::string base32_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    static const std::vector<int> base32_lookup = [] {
        std::vector<int> lookup(256, -1);
        for (size_t i = 0; i < base32_chars.size(); ++i) {
            lookup[static_cast<unsigned char>(base32_chars[i])] = static_cast<int>(i);
        }
        return lookup;
    }();

    if (input.empty()) {
        return {};
    }

    // Check that the input length is a multiple of 8
    if (input.size() % 8 != 0) {
        throw std::invalid_argument("Base32 input length must be a multiple of 8");
    }

    std::string result;
    result.reserve((input.size() * 5) / 8);

    uint32_t buffer = 0;
    int bits_in_buffer = 0;

    for (char c : input) {
        if (c == '=') {
            break;  // Padding character found, stop processing
        }

        c = std::toupper(c);  // Ensure case insensitivity
        if (base32_lookup[static_cast<unsigned char>(c)] == -1) {
            throw std::invalid_argument("Invalid Base32 character");
        }

        buffer = (buffer << 5) | base32_lookup[static_cast<unsigned char>(c)];
        bits_in_buffer += 5;

        if (bits_in_buffer >= 8) {
            bits_in_buffer -= 8;
            result.push_back(static_cast<char>((buffer >> bits_in_buffer) & 0xFF));
        }
    }

    return result;
}

std::string IntToBytes(size_t counter) {
    std::string result(8, '\0');

    for (int i = 7; i >= 0; --i) {
        result[i] = counter & 0xFF;
        counter >>= 8;
    }

    return result;
}

uint32_t CalculateDynamicBinaryCode(const std::string& secret_base32, uint64_t counter) {
    const std::string counter_bytes = IntToBytes(counter);
    const std::string secret = Base32Decode(secret_base32);
    const std::string hmac_result = userver::crypto::base64::Base64Decode(
        userver::crypto::hash::HmacSha1(secret, counter_bytes, userver::crypto::hash::OutputEncoding::kBase64)
    );

    const uint32_t offset = hmac_result.back() & 0x0F;

    uint32_t dbc = 0;
    dbc |= (static_cast<uint8_t>(hmac_result[offset + 0]) & 0x7F) << 24;
    dbc |= (static_cast<uint8_t>(hmac_result[offset + 1]) & 0xFF) << 16;
    dbc |= (static_cast<uint8_t>(hmac_result[offset + 2]) & 0xFF) << 8;
    dbc |= (static_cast<uint8_t>(hmac_result[offset + 3]) & 0xFF) << 0;

    return dbc;
}

// Precomputed powers of 10 for digits 6 to 8
constexpr std::array<uint32_t, 3> kPowersOf10 = {1000000, 10000000, 100000000};

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

uint32_t GenerateTotpCode(const std::string& secret, uint32_t period, size_t digits, std::time_t timestamp) {
    if (digits < 6 || digits > 8) {
        throw std::invalid_argument("TOTP code must have between 6 and 8 digits");
    }

    const uint64_t counter = timestamp / period;
    const uint32_t dbc = CalculateDynamicBinaryCode(secret, counter);

    return dbc % kPowersOf10[digits - 6];
}

bool VerifyTotpCode(
    const std::string& secret,
    uint32_t totp_code,
    uint32_t period,
    size_t digits,
    int window,
    std::time_t timestamp
) {
    const uint64_t counter = timestamp / period;

    for (int i = -window; i <= window; ++i) {
        const uint64_t test_counter = counter + i;
        const uint32_t dbc = CalculateDynamicBinaryCode(secret, test_counter);

        if (dbc % kPowersOf10[digits - 6] == totp_code) {
            return true;
        }
    }

    return false;
}

}  // namespace totp