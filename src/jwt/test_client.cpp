#include "client.hpp"

#include <userver/utest/utest.hpp>

using namespace jwt;

/// Test the constructor of Client
TEST(JwtClientTest, ConstructorInitialization) {
    std::string secret_key = "test_secret_key";
    std::chrono::milliseconds token_ttl(60000);  // 1 minute

    Client client(secret_key, token_ttl);
    // No specific assertions needed, just ensuring no exceptions are thrown
}

/// Test GenerateToken
TEST(JwtClientTest, GenerateToken_ValidPayload) {
    std::string secret_key = "test_secret_key";
    std::chrono::milliseconds token_ttl(60000);  // 1 minute

    Client client(secret_key, token_ttl);

    Payload payload = {.user_id = 12345, .master_key = "test_master_key"};

    std::string token = client.GenerateToken(payload);
    EXPECT_FALSE(token.empty());
}

/// Test ValidateToken with valid token
TEST(JwtClientTest, ValidateToken_ValidToken) {
    std::string secret_key = "test_secret_key";
    std::chrono::milliseconds token_ttl(60000);  // 1 minute

    Client client(secret_key, token_ttl);

    Payload payload = {.user_id = 12345, .master_key = "test_master_key"};

    std::string token = client.GenerateToken(payload);
    auto parsed_payload = client.ValidateToken(token);

    EXPECT_EQ(parsed_payload.user_id, payload.user_id);
    EXPECT_EQ(parsed_payload.master_key, payload.master_key);
}

/// Test ValidateToken with expired token
TEST(JwtClientTest, ValidateToken_ExpiredToken) {
    std::string secret_key = "test_secret_key";
    std::chrono::milliseconds token_ttl(-60000);  // Token immediately expires

    Client client(secret_key, token_ttl);

    Payload payload = {.user_id = 12345, .master_key = "test_master_key"};

    std::string token = client.GenerateToken(payload);

    // Validating an expired token should throw an exception
    EXPECT_THROW(client.ValidateToken(token), std::runtime_error);
}

/// Test ValidateToken with invalid token
TEST(JwtClientTest, ValidateToken_InvalidToken) {
    std::string secret_key = "test_secret_key";
    std::chrono::milliseconds token_ttl(60000);  // 1 minute

    Client client(secret_key, token_ttl);

    std::string invalid_token = "invalid.token.value";

    // Validating an invalid token should throw an exception
    EXPECT_THROW(client.ValidateToken(invalid_token), std::runtime_error);
}
