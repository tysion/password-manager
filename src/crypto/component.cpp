#include "component.hpp"

#include <userver/components/component.hpp>
#include <userver/crypto/base64.hpp>
#include <userver/utils/algo.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace crypto {

Component::Component(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
)
    : userver::components::LoggableComponentBase(config, context), storage_{config.As<Component::Storage>()} {}

std::string Component::GetDecodedKey(const std::string& name) const {
    const std::string* key_encoded = userver::utils::FindOrNullptr(storage_, name);
    if (!key_encoded) {
        throw std::runtime_error("Unknown crypto key");
    }

    return userver::crypto::base64::Base64Decode(*key_encoded);
}

userver::yaml_config::Schema Component::GetStaticConfigSchema() {
    constexpr auto schema = R"(
        type: object
        description: jwt component
        additionalProperties: false
        properties:
            aes256_base64_key:
                type: string
                description: AES-256 key for working with master key stored in JWT token
    )";
    return userver::yaml_config::MergeSchemas<userver::components::LoggableComponentBase>(schema);
}

}  // namespace crypto