#pragma once

#include <userver/components/loggable_component_base.hpp>

#include <unordered_map>

namespace crypto {

class Component final : public userver::components::LoggableComponentBase {
public:
    static constexpr std::string_view kName = "component-crypto";

    Component(const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context);

    std::string GetDecodedKey(const std::string& name) const;

    static userver::yaml_config::Schema GetStaticConfigSchema();

private:
    using Storage = std::unordered_map<std::string, std::string>;
    const Storage storage_;
};
}  // namespace crypto