#pragma once

#include <memory>
#include <string>

namespace Animation {
    struct SpriteAnimationSet;
}

namespace Core {
    struct EngineContext;
}

namespace Rendering {
    class Texture;
    class Shader;
    class Mesh;
    struct Material;
} // namespace Rendering

namespace UI {
    class Font;
}

namespace Editor::UI {

    bool TextureCombo(const char *label, Core::EngineContext *context, std::shared_ptr<Rendering::Texture> &current);

    bool ShaderCombo(const char *label, Core::EngineContext *context, std::shared_ptr<Rendering::Shader> &current);

    bool MeshCombo(const char *label, Core::EngineContext *context, std::shared_ptr<Rendering::Mesh> &current);

    bool MaterialCombo(const char *label, Core::EngineContext *context, std::shared_ptr<Rendering::Material> &current);

    bool FontCombo(const char *label, Core::EngineContext *context, std::shared_ptr<::UI::Font> &current);

    bool AnimationCombo(const char *label, Core::EngineContext *context, std::shared_ptr<const Animation::SpriteAnimationSet> &current);

    bool FileCombo(const char *label, const std::string &subDir, const std::string &extension, std::string &current);

} // namespace Editor::UI
