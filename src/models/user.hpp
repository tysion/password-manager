#pragma once

#include <chrono>
#include <string>

namespace models {

struct User final {
    std::int32_t id;
    std::string username;
    std::string master_key_hash;
    std::string salt_encoded;
    std::string totp_secret;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
};

}  // namespace models