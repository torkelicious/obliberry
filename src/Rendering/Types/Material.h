#pragma once

#include <memory>
#include "Shader/Shader.h"
#include "Texture/Texture.h"


namespace Rendering {
    struct Material {
        std::shared_ptr<Shader> shader;
        std::shared_ptr<Texture> texture;
        glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
    };
} // namespace Rendering
