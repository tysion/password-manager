#include "handler.hpp"
#include "crypto/component.hpp"
#include "crypto/utils.hpp"
#include "db/sql.hpp"
#include "jwt/component.hpp"
#include "models/user.hpp"
#include "totp/utils.hpp"

#include <userver/components/component.hpp>
#include <userver/crypto/base64.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

namespace {

userver::formats::json::Value UnknownUser() {
    userver::formats::json::ValueBuilder builder;
    builder["message"] = "Unknown user";
    return builder.ExtractValue();
}

userver::formats::json::Value InvalidMasterKeyOrTotpCode() {
    userver::formats::json::ValueBuilder builder;
    builder["message"] = "Invalid master key or TOTP code";
    return builder.ExtractValue();
}

}  // namespace

namespace handlers::api::login::post {

Handler::Handler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_{context.FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()},
      jwt_client_{context.FindComponent<jwt::Component>().GetClient()},
      key_{context.FindComponent<crypto::Component>().GetDecodedKey("aes256_base64_key")} {}

userver::formats::json::Value Handler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest& request,
    const userver::formats::json::Value& body,
    [[maybe_unused]] userver::server::request::RequestContext& context
) const {
    LOG_INFO() << "Received authentication request";

    const auto username = body["username"].As<std::string>();
    LOG_DEBUG() << "Username: " << username;

    const auto master_key_encoded = body["master_key"].As<std::string>();
    const auto totp_code = body["totp_code"].As<uint32_t>();
    const auto master_key = userver::crypto::base64::Base64Decode(master_key_encoded);
    LOG_DEBUG() << "Decoded master key and TOTP code";

    // Fetch user from database
    const auto result =
        pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kSlave, db::sql::kGetUser, username);

    if (result.IsEmpty()) {
        LOG_WARNING() << "Unknown user: " << username;
        request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
        return UnknownUser();
    }

    const auto user = result.AsSingleRow<models::User>(userver::storages::postgres::kRowTag);
    LOG_DEBUG() << "User found in database: " << user.id;

    // verify master key
    const auto salt = userver::crypto::base64::Base64Decode(user.salt_encoded);
    if (!crypto::VerifyMasterKeyHashWithSalt(master_key, salt, user.master_key_hash)) {
        LOG_WARNING() << "Invalid master key for user: " << username;
        request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
        return InvalidMasterKeyOrTotpCode();
    }

    // verify TOTP code
    if (!totp::VerifyTotpCode(user.totp_secret, totp_code)) {
        LOG_WARNING() << "Invalid TOTP code for user: " << username;
        request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
        return InvalidMasterKeyOrTotpCode();
    }

    jwt::Payload jwt_payload;
    jwt_payload.user_id = user.id;
    jwt_payload.master_key = crypto::Encrypt(master_key, key_);
    const auto token = jwt_client_.GenerateToken(jwt_payload);

    LOG_INFO() << "JWT token generated successfully for user: " << username;

    userver::formats::json::ValueBuilder builder;
    builder["token"] = token;
    return builder.ExtractValue();
}

}  // namespace handlers::api::login::post
