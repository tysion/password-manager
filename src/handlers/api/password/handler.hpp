#pragma once

#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/storages/postgres/postgres_fwd.hpp>

namespace jwt {

class Client;

}  // namespace jwt

namespace handlers::api::password::get {

class Handler final : public userver::server::handlers::HttpHandlerJsonBase {
public:
    static constexpr std::string_view kName = "handler-get-password";

    Handler(const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context);

    userver::formats::json::Value HandleRequestJsonThrow(
        const userver::server::http::HttpRequest& request,
        const userver::formats::json::Value& body,
        userver::server::request::RequestContext& context
    ) const override;

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
    const std::string key_;
};

}  // namespace handlers::api::password::get

namespace handlers::api::passwords::get {

class Handler final : public userver::server::handlers::HttpHandlerJsonBase {
public:
    static constexpr std::string_view kName = "handler-get-passwords";

    Handler(const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context);

    userver::formats::json::Value HandleRequestJsonThrow(
        const userver::server::http::HttpRequest& request,
        const userver::formats::json::Value& body,
        userver::server::request::RequestContext& context
    ) const override;

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
    const std::string key_;
};

}  // namespace handlers::api::passwords::get

namespace handlers::api::password::post {

class Handler final : public userver::server::handlers::HttpHandlerJsonBase {
public:
    static constexpr std::string_view kName = "handler-post-password";

    Handler(const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context);

    userver::formats::json::Value HandleRequestJsonThrow(
        const userver::server::http::HttpRequest& request,
        const userver::formats::json::Value& body,
        userver::server::request::RequestContext& context
    ) const override;

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
    const std::string key_;
};

}  // namespace handlers::api::password::post
