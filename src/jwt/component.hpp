#pragma once

#include "client.hpp"

#include <userver/components/loggable_component_base.hpp>

namespace jwt {

class Component final : public userver::components::LoggableComponentBase {
public:
    static constexpr std::string_view kName = "component-jwt";

    Component(const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context);

    const Client& GetClient();

    static userver::yaml_config::Schema GetStaticConfigSchema();

private:
    const Client client_;
};

}  // namespace jwt