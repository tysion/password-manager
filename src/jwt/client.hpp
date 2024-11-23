#pragma once

#include <chrono>
#include <string>

namespace jwt {

/// Represents the payload of a JWT token.
struct Payload {
    /// User ID associated with the token.
    std::int32_t user_id;

    /// Master key used for encryption/decryption.
    std::string master_key;
};

/// Provides functionality for generating and validating JWT tokens.
class Client {
public:
    /// Constructs a JWT client.
    /// @param secret_key The secret key used for signing tokens.
    /// @param token_ttl The time-to-live (TTL) for generated tokens.
    Client(std::string secret_key, std::chrono::milliseconds token_ttl);

    /// Generates a JWT token from the given payload.
    /// @param payload The payload to embed in the token.
    /// @return The generated JWT token as a string.
    std::string GenerateToken(const Payload& payload) const;

    /// Validates a JWT token and extracts the payload.
    /// @param token The JWT token to validate.
    /// @return The extracted payload if the token is valid.
    /// @throws std::runtime_error If the token is invalid or expired.
    Payload ValidateToken(const std::string& token) const;

private:
    /// Secret key used for signing tokens.
    const std::string secret_key_;

    /// Time-to-live (TTL) for generated tokens.
    const std::chrono::milliseconds token_ttl_;
};

}  // namespace jwt