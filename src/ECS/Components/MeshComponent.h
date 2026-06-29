#pragma once

#include <memory>

namespace Rendering {
    class Mesh;
}

namespace ECS::Components {
    struct MeshComponent {
        std::shared_ptr<Rendering::Mesh> mesh;
    };
} // namespace ECS::Components
