#include "handler.hpp"
#include "crypto/component.hpp"
#include "crypto/utils.hpp"
#include "db/sql.hpp"
#include "models/password.hpp"

#include <userver/components/component.hpp>
#include <userver/crypto/base64.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

namespace {

class Forbidden
    : public userver::server::handlers::ExceptionWithCode<userver::server::handlers::HandlerErrorCode::kForbidden> {
public:
    using BaseType::BaseType;
};

userver::formats::json::Value SerializePassword(const models::Password& password, const std::string& decrypted) {
    userver::formats::json::ValueBuilder builder;
    builder["id"] = password.id;
    builder["user_id"] = password.user_id;
    builder["service"] = password.service;
    builder["login"] = password.login;
    builder["password"] = decrypted;
    builder["created_at"] = password.created_at;
    builder["updated_at"] = password.updated_at;
    return builder.ExtractValue();
}

}  // namespace

namespace handlers::api::password::get {

Handler::Handler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_{context.FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()},
      key_{context.FindComponent<crypto::Component>().GetDecodedKey("aes256_base64_key")} {}

userver::formats::json::Value Handler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest& request,
    [[maybe_unused]] const userver::formats::json::Value& body,
    userver::server::request::RequestContext& context
) const {
    LOG_INFO() << "Received request to retrieve a password";

    const auto user_id = context.GetData<std::int32_t>("user_id");
    const auto master_key = crypto::Decrypt(context.GetData<std::string>("master_key"), key_);

    const auto password_id = std::stoll(request.GetPathArg("id"));

    const auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kSlave, db::sql::kGetPassword, password_id, user_id
    );

    if (result.IsEmpty()) {
        LOG_WARNING() << "Password not found for ID: " << password_id;
        throw userver::server::handlers::ResourceNotFound(userver::server::handlers::ExternalBody{"Password not found"}
        );
    }

    const auto password = result.AsSingleRow<models::Password>(userver::storages::postgres::kRowTag);
    if (password.user_id != user_id) {
        LOG_WARNING() << "Access denied for password ID: " << password_id;
        throw Forbidden(userver::server::handlers::ExternalBody{"Access denied"});
    }

    const auto password_decrypted =
        crypto::Decrypt(userver::crypto::base64::Base64Decode(password.password_encrypted), master_key);

    LOG_DEBUG() << "Password decrypted successfully for ID: " << password_id;

    const auto response = SerializePassword(password, password_decrypted);
    LOG_INFO() << "Password retrieved successfully for ID: " << password_id;

    return response;
}

}  // namespace handlers::api::password::get

namespace handlers::api::passwords::get {

Handler::Handler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_{context.FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()},
      key_{context.FindComponent<crypto::Component>().GetDecodedKey("aes256_base64_key")} {}

userver::formats::json::Value Handler::HandleRequestJsonThrow(
    [[maybe_unused]] const userver::server::http::HttpRequest& request,
    [[maybe_unused]] const userver::formats::json::Value& body,
    userver::server::request::RequestContext& context
) const {
    LOG_INFO() << "Received request to retrieve passwords";

    const auto user_id = context.GetData<std::int32_t>("user_id");
    const auto master_key = crypto::Decrypt(context.GetData<std::string>("master_key"), key_);

    const auto result =
        pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kSlave, db::sql::kGetPasswords, user_id);

    const auto passwords = result.AsContainer<std::vector<models::Password>>(userver::storages::postgres::kRowTag);

    userver::formats::json::ValueBuilder response(userver::formats::common::Type::kArray);
    for (const auto& password : passwords) {
        const auto password_decrypted =
            crypto::Decrypt(userver::crypto::base64::Base64Decode(password.password_encrypted), master_key);

        LOG_DEBUG() << "Password decrypted successfully for ID: " << password.id;

        response.PushBack(SerializePassword(password, password_decrypted));
    }

    LOG_INFO() << "Passwords retrieved successfully";

    return response.ExtractValue();
}

}  // namespace handlers::api::passwords::get

namespace handlers::api::password::post {

Handler::Handler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_{context.FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()},
      key_{context.FindComponent<crypto::Component>().GetDecodedKey("aes256_base64_key")} {}

userver::formats::json::Value Handler::HandleRequestJsonThrow(
    [[maybe_unused]] const userver::server::http::HttpRequest& request,
    const userver::formats::json::Value& body,
    userver::server::request::RequestContext& context
) const {
    LOG_INFO() << "Received request to create a password";

    const auto user_id = context.GetData<std::int32_t>("user_id");
    const auto master_key = crypto::Decrypt(context.GetData<std::string>("master_key"), key_);

    const auto service = body["service"].As<std::string>();
    const auto login = body["login"].As<std::string>();
    const auto password = body["password"].As<std::string>();
    const auto password_encrypted = userver::crypto::base64::Base64Encode(crypto::Encrypt(password, master_key));
    LOG_DEBUG() << "Password emcrypted successfully";

    const auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        db::sql::kCreatePassword,
        user_id,
        service,
        login,
        password_encrypted
    );

    LOG_INFO() << "Password created successfully";

    userver::formats::json::ValueBuilder response;
    response["message"] = "Password added successfully";
    return response.ExtractValue();
}

}  // namespace handlers::api::password::post

namespace handlers::api::password::del {

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
    LOG_INFO() << "Received request to delete a password";

    const auto user_id = context.GetData<std::int32_t>("user_id");
    const auto password_id = std::stoll(request.GetPathArg("id"));

    const auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster, db::sql::kDeletePassword, password_id, user_id
    );

    if (result.RowsAffected() == 0) {
        LOG_WARNING() << "Password not found for ID: " << password_id;
        throw userver::server::handlers::ResourceNotFound(userver::server::handlers::ExternalBody{"Password not found"}
        );
    }

    LOG_INFO() << "Password deleted successfully";

    userver::formats::json::ValueBuilder response;
    response["message"] = "Password deleted successfully";
    return response.ExtractValue();
}

}  // namespace handlers::api::password::del