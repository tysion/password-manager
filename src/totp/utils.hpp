#pragma once

#include <ctime>
#include <string>

namespace totp {

/// @brief Generates a secret key for TOTP.
///
/// The secret key is a randomly generated alphanumeric string.
///
/// @param length Length of the secret key (default is 32 characters).
/// @return A randomly generated secret key as a string.
std::string GenerateTotpSecret(size_t length = 32);

/// @brief Generates a TOTP code based on the current time and the given secret
/// key.
///
/// The TOTP code is derived using the HMAC-SHA1 algorithm and is valid for a
/// specific time period.
///
/// @param secret The secret key.
/// @param period The validity period of the TOTP code in seconds (default is 30
/// seconds).
/// @param digits The number of digits in the TOTP code (default is 6 digits).
/// @return The generated TOTP code.
uint32_t GenerateTotpCode(
    const std::string& secret,
    uint32_t period = 30,
    size_t digits = 6,
    std::time_t timestamp = std::time(nullptr)
);

/// @brief Verifies the correctness of a given TOTP code.
///
/// This function checks if the provided TOTP code matches the expected value
/// for the current or nearby time intervals.
///
/// @param secret The secret key.
/// @param totp_code The TOTP code to verify.
/// @param period The validity period of the TOTP code in seconds (default is 30
/// seconds).
/// @param digits The number of digits in the TOTP code (default is 6 digits).
/// @param window The allowed time window (number of intervals to check before
/// and after, default is 1).
/// @return true if the code is valid; false otherwise.
bool VerifyTotpCode(
    const std::string& secret,
    uint32_t totp_code,
    uint32_t period = 30,
    size_t digits = 6,
    int window = 1,
    std::time_t timestamp = std::time(nullptr)
);

}  // namespace totp