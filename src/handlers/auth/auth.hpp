#pragma once

#include <userver/server/handlers/auth/auth_checker_factory.hpp>

namespace jwt {
class Client;
}

namespace handlers::auth {

class AuthChecker final : public userver::server::handlers::auth::AuthCheckerBase {
public:
    using Result = userver::server::handlers::auth::AuthCheckResult;

    AuthChecker(const jwt::Client& jwt_client);

    Result CheckAuth(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& request_context
    ) const override;

    bool SupportsUserAuth() const noexcept override { return true; }

private:
    const jwt::Client& jwt_client_;
};

class AuthCheckerFactory final : public userver::server::handlers::auth::AuthCheckerFactoryBase {
public:
    userver::server::handlers::auth::AuthCheckerBasePtr operator()(
        const userver::components::ComponentContext& context,
        const userver::server::handlers::auth::HandlerAuthConfig& auth_config,
        const userver::server::handlers::auth::AuthCheckerSettings& settings
    ) const override;
};

}  // namespace handlers::auth