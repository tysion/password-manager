#include "utils.hpp"

#include <userver/utest/utest.hpp>

class TotpUtilsTest : public ::testing::Test {
protected:
    std::string secret = totp::GenerateTotpSecret();

    std::time_t fixed_time = 1672531200;  // 2023-01-01 00:00:00 UTC

    void SetUp() override { ASSERT_FALSE(secret.empty()); }
};

TEST_F(TotpUtilsTest, GenerateTotpSecretLength) {
    size_t byte_length = 16;
    size_t base32_length = ((byte_length * 8) + 4) / 5;
    base32_length = ((base32_length + 7) / 8) * 8;

    std::string generated_secret = totp::GenerateTotpSecret(byte_length);
    ASSERT_EQ(generated_secret.size(), base32_length);
}

TEST_F(TotpUtilsTest, GenerateTotpSecretDefaultLength) {
    size_t byte_length = 32;
    size_t base32_length = ((byte_length * 8) + 4) / 5;
    base32_length = ((base32_length + 7) / 8) * 8;

    std::string default_secret = totp::GenerateTotpSecret();
    ASSERT_EQ(default_secret.size(), base32_length);
}

TEST_F(TotpUtilsTest, GenerateTotpCodeValidDigits) {
    const uint32_t totp_code_6 = totp::GenerateTotpCode(secret, 30, 6);
    ASSERT_GE(totp_code_6, 100000);
    ASSERT_LT(totp_code_6, 1000000);

    const uint32_t totp_code_8 = totp::GenerateTotpCode(secret, 30, 8);
    ASSERT_GE(totp_code_8, 10000000);
    ASSERT_LT(totp_code_8, 100000000);
}

TEST_F(TotpUtilsTest, GenerateTotpCodeInvalidDigits) {
    const std::string secret = totp::GenerateTotpSecret();

    EXPECT_THROW(totp::GenerateTotpCode(secret, 30, 5), std::invalid_argument);
    EXPECT_THROW(totp::GenerateTotpCode(secret, 30, 9), std::invalid_argument);
}

TEST_F(TotpUtilsTest, VerifyTotpCodeValid) {
    uint32_t totp_code = totp::GenerateTotpCode(secret, 30, 6);
    ASSERT_TRUE(totp::VerifyTotpCode(secret, totp_code, 30, 6));
}

TEST_F(TotpUtilsTest, VerifyTotpCodeInvalid) {
    uint32_t invalid_code = 123456;
    ASSERT_FALSE(totp::VerifyTotpCode(secret, invalid_code, 30, 6));
}

TEST_F(TotpUtilsTest, VerifyTotpCodeWithWindow) {
    uint32_t totp_code = totp::GenerateTotpCode(secret, 30, 6);
    ASSERT_TRUE(totp::VerifyTotpCode(secret, totp_code, 30, 6, 1));
}

TEST_F(TotpUtilsTest, VerifyTotpCodeOutsideWindow) {
    uint32_t totp_code = totp::GenerateTotpCode(secret, 30, 6);
    ASSERT_FALSE(totp::VerifyTotpCode(secret, totp_code, 30, 6, 1, fixed_time));
}
