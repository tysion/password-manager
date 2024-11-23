#include "client.hpp"

#include <userver/crypto/base64.hpp>
#include <userver/crypto/hash.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utils/text_light.hpp>

namespace {

std::time_t GetExpirationTimestamp(std::chrono::milliseconds ttl) {
    const auto timepoint = std::chrono::system_clock::now() + ttl;
    return std::chrono::system_clock::to_time_t(timepoint);
}

}  // namespace

namespace jwt {

Client::Client(std::string secret_key, std::chrono::milliseconds token_ttl)
    : secret_key_{std::move(secret_key)}, token_ttl_{std::move(token_ttl)} {}

std::string Client::GenerateToken(const Payload& payload) const {
    // encode header
    userver::formats::json::ValueBuilder header;
    header["alg"] = "HS256";
    header["typ"] = "JWT";
    header["exp"] = GetExpirationTimestamp(token_ttl_);
    const auto encoded_header =
        userver::crypto::base64::Base64Encode(userver::formats::json::ToString(header.ExtractValue()));

    // encode payload
    userver::formats::json::ValueBuilder body;
    body["user_id"] = payload.user_id;
    body["master_key"] = payload.master_key;
    const auto encoded_body =
        userver::crypto::base64::Base64Encode(userver::formats::json::ToString(body.ExtractValue()));

    // signature
    const auto data = encoded_header + "." + encoded_body;
    const auto signature =
        userver::crypto::hash::HmacSha256(data, secret_key_, userver::crypto::hash::OutputEncoding::kBase64);

    return encoded_header + "." + encoded_body + "." + signature;
}

Payload Client::ValidateToken(const std::string& token) const {
    auto parts = userver::utils::text::Split(token, ".");
    if (parts.size() != 3) {
        throw std::runtime_error("Invalid JWT format");
    }

    const auto encoded_header = std::move(parts[0]);
    const auto encoded_body = std::move(parts[1]);
    const auto encoded_signature = std::move(parts[2]);

    // signature check
    const auto data = encoded_header + "." + encoded_body;
    const auto expected_signature =
        userver::crypto::hash::HmacSha256(data, secret_key_, userver::crypto::hash::OutputEncoding::kBase64);
    if (expected_signature != encoded_signature) {
        throw std::runtime_error("Invalid JWT signature");
    }

    const auto decoded_header =
        userver::formats::json::FromString(userver::crypto::base64::Base64Decode(encoded_header));

    // exp check
    const auto exp = decoded_header["exp"].As<std::time_t>();
    const auto timepoint = std::chrono::system_clock::now();
    if (std::chrono::system_clock::to_time_t(timepoint) > exp) {
        throw std::runtime_error("JWT has expired");
    }

    const auto decoded_body = userver::formats::json::FromString(userver::crypto::base64::Base64Decode(encoded_body));
    Payload payload;
    payload.user_id = decoded_body["user_id"].As<int32_t>();
    payload.master_key = decoded_body["master_key"].As<std::string>();

    return payload;
}

}  // namespace jwt