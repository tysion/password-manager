#include "auth.hpp"
#include "jwt/component.hpp"

#include <userver/http/common_headers.hpp>
#include <userver/logging/log.hpp>

namespace handlers::auth {

constexpr std::string_view kAuthHeaderPrefix = "Bearer ";

AuthChecker::AuthChecker(const jwt::Client& jwt_client) : jwt_client_{jwt_client} {}

AuthChecker::Result AuthChecker::CheckAuth(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& request_context
) const {
    const auto auth_header = request.GetHeader(userver::http::headers::kAuthorization);
    if (!auth_header.starts_with(kAuthHeaderPrefix)) {
        LOG_WARNING() << "Authorization header missing or invalid";
        Result result;
        result.status = Result::Status::kTokenNotFound;
        result.ext_reason = "Invalid Authorization header";
        result.code = userver::server::handlers::HandlerErrorCode::kUnauthorized;
        return result;
    }
    const auto token = auth_header.substr(kAuthHeaderPrefix.size());

    jwt::Payload jwt_payload;
    try {
        jwt_payload = jwt_client_.ValidateToken(token);
        LOG_DEBUG() << "Token validated for user ID: " << jwt_payload.user_id;
    } catch (const std::exception& ex) {
        LOG_WARNING() << "JWT validation failed: " << ex.what();
        Result result;
        result.status = Result::Status::kTokenNotFound;
        result.reason = "Invalid token";
        result.ext_reason = ex.what();
        result.code = userver::server::handlers::HandlerErrorCode::kUnauthorized;
        return result;
    }

    request_context.SetData("master_key", jwt_payload.master_key);
    request_context.SetData("user_id", jwt_payload.user_id);
    return {};
}

userver::server::handlers::auth::AuthCheckerBasePtr AuthCheckerFactory::operator()(
    const userver::components::ComponentContext& context,
    [[maybe_unused]] const userver::server::handlers::auth::HandlerAuthConfig& config,
    [[maybe_unused]] const userver::server::handlers::auth::AuthCheckerSettings& settings
) const {
    const auto& jwt_client = context.FindComponent<jwt::Component>().GetClient();
    return std::make_shared<AuthChecker>(jwt_client);
}

}  // namespace handlers::auth
