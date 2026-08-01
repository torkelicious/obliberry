#pragma once

#include "ECS/Types.h"
#include <string>
#include <vector>

namespace ECS::Components {
    struct RelationshipComponent {
        EntityID parent = INVALID_ENTITY_ID;
        std::string parentName; // for serialization
        std::vector<EntityID> children;
    };
} // namespace ECS::Components
