#pragma once

#include "ECS/Types.h"
#include <string>
#include <vector>

namespace ECS::Components {
    struct RelationshipComponent {
        EntityID parent = 0;
        std::string parentName; // for serialization
        std::vector<EntityID> children;
    };
} // namespace ECS::Components
