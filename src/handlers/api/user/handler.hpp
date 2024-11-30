#pragma once

#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/storages/postgres/postgres_fwd.hpp>

namespace handlers::api::user::post {

class Handler final : public userver::server::handlers::HttpHandlerJsonBase {
public:
    static constexpr std::string_view kName = "handler-post-user";

    Handler(const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context);

    userver::formats::json::Value HandleRequestJsonThrow(
        const userver::server::http::HttpRequest& request,
        const userver::formats::json::Value& body,
        userver::server::request::RequestContext& context
    ) const override;

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace handlers::api::user::post

namespace handlers::api::user::del {

class Handler final : public userver::server::handlers::HttpHandlerJsonBase {
public:
    static constexpr std::string_view kName = "handler-delete-user";

    Handler(const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context);

    userver::formats::json::Value HandleRequestJsonThrow(
        const userver::server::http::HttpRequest& request,
        const userver::formats::json::Value& body,
        userver::server::request::RequestContext& context
    ) const override;

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace handlers::api::user::del
