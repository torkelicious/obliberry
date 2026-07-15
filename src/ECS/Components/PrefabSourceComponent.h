#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace ECS::Components {
    struct PrefabSourceComponent {
        std::string prefabPath;
        nlohmann::json originalData; // snapshot of the prefab data at instantiation
    };
} // namespace ECS::Components
