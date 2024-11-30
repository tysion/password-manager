#include "crypto/component.hpp"
#include "handlers/api/login/handler.hpp"
#include "handlers/api/password/handler.hpp"
#include "handlers/api/user/handler.hpp"
#include "handlers/auth/auth.hpp"
#include "jwt/component.hpp"

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/auth/auth_checker_factory.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

int main(int argc, char* argv[]) {
    userver::server::handlers::auth::RegisterAuthCheckerFactory(
        "bearer", std::make_unique<handlers::auth::AuthCheckerFactory>()
    );

    auto component_list = userver::components::MinimalServerComponentList()
                              .Append<userver::server::handlers::Ping>()
                              .Append<userver::server::handlers::TestsControl>()
                              .Append<userver::server::handlers::ServerMonitor>()
                              .Append<userver::components::TestsuiteSupport>()
                              .Append<userver::components::HttpClient>()
                              .Append<userver::components::Postgres>("postgres-db-1")
                              .Append<userver::clients::dns::Component>()
                              .Append<handlers::api::user::post::Handler>()
                              .Append<handlers::api::login::post::Handler>()
                              .Append<handlers::api::password::get::Handler>()
                              .Append<handlers::api::passwords::get::Handler>()
                              .Append<handlers::api::password::post::Handler>()
                              .Append<handlers::api::password::del::Handler>()
                              .Append<jwt::Component>()
                              .Append<crypto::Component>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
