#include "handler.hpp"
#include "crypto/utils.hpp"
#include "db/sql.hpp"
#include "models/user.hpp"
#include "totp/utils.hpp"

#include <userver/components/component.hpp>
#include <userver/crypto/base64.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

namespace handlers::api::user::post {

Handler::Handler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_{context.FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()} {}

userver::formats::json::Value Handler::HandleRequestJsonThrow(
    [[maybe_unused]] const userver::server::http::HttpRequest& request,
    const userver::formats::json::Value& body,
    [[maybe_unused]] userver::server::request::RequestContext& context
) const {
    LOG_INFO() << "Received request to register a new user";

    const auto username = body["username"].As<std::string>();
    LOG_DEBUG() << "Username extracted: " << username;

    try {
        const auto master_key = crypto::GenerateMasterKey();
        const auto salt = crypto::GenerateSalt();
        const auto master_key_hash = crypto::HashMasterKeyWithSalt(master_key, salt);
        const auto totp_secret = totp::GenerateTotpSecret();
        const auto master_key_encoded = userver::crypto::base64::Base64Encode(master_key);
        const auto salt_encoded = userver::crypto::base64::Base64Encode(salt);

        const auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            db::sql::kCreateUser,
            username,
            master_key_hash,
            salt_encoded,
            totp_secret
        );

        LOG_INFO() << "User successfully created in database: " << username;

        userver::formats::json::ValueBuilder builder;
        builder["message"] = "User registered successfully";
        builder["master_key"] = master_key_encoded;
        builder["totp_secret"] = totp_secret;

        return builder.ExtractValue();

    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error while registering user: " << ex.what();
        throw;
    }
}

}  // namespace handlers::api::user::post

namespace handlers::api::user::del {

Handler::Handler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_{context.FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()} {}

userver::formats::json::Value Handler::HandleRequestJsonThrow(
    [[maybe_unused]] const userver::server::http::HttpRequest& request,
    const userver::formats::json::Value& body,
    [[maybe_unused]] userver::server::request::RequestContext& context
) const {
    LOG_INFO() << "Received request to delete a user";

    const auto username = body["username"].As<std::string>();
    const auto totp_code = std::stoul(body["totp_code"].As<std::string>());
    LOG_DEBUG() << "Username extracted: " << username;

    const auto get_result =
        pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kSlave, db::sql::kGetUser, username);

    if (get_result.IsEmpty()) {
        LOG_WARNING() << "Unknown user: " << username;
        throw userver::server::handlers::Unauthorized(userver::server::handlers::ExternalBody{"Unknown user"});
    }

    const auto user = get_result.AsSingleRow<models::User>(userver::storages::postgres::kRowTag);
    LOG_DEBUG() << "User found in database: " << user.id;

    if (!totp::VerifyTotpCode(user.totp_secret, totp_code)) {
        LOG_WARNING() << "Invalid TOTP code for user: " << username;
        throw userver::server::handlers::Unauthorized(userver::server::handlers::ExternalBody{"Invalid TOTP code"});
    }

    const auto delete_result =
        pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kMaster, db::sql::kDeleteUser, username);

    if (delete_result.RowsAffected() == 0) {
        LOG_WARNING() << "Failed to delete user: " << username;
        throw userver::server::handlers::InternalServerError(
            userver::server::handlers::ExternalBody{"Failed to delete user"}
        );
    }

    LOG_INFO() << "User successfully deleted from database: " << username;

    userver::formats::json::ValueBuilder builder;
    builder["message"] = "User deleted successfully";

    return builder.ExtractValue();
}

}  // namespace handlers::api::user::del
