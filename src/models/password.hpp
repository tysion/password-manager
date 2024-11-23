#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace models {

struct Password final {
    std::int32_t id;
    std::int32_t user_id;
    std::string service;
    std::string login;
    std::string password_encrypted;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
};

}  // namespace models