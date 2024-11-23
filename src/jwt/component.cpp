#include "component.hpp"

#include <userver/components/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace jwt {

Component::Component(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
)
    : userver::components::LoggableComponentBase(config, context),
      client_(config["secret_key"].As<std::string>(), config["token_ttl"].As<std::chrono::milliseconds>()) {}

const Client& Component::GetClient() { return client_; }

userver::yaml_config::Schema Component::GetStaticConfigSchema() {
    constexpr auto schema = R"(
        type: object
        description: jwt component
        additionalProperties: false
        properties:
            secret_key:
                type: string
                description: jwt secret key
            token_ttl:
                type: string
                description: jwt token ttl (exp)
    )";
    return userver::yaml_config::MergeSchemas<userver::components::LoggableComponentBase>(schema);
}

}  // namespace jwt