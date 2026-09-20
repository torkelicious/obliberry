#include "EditorWidgetsCombo.h"

#include "Core/EngineContext.h"
#include "Core/ResourceManager.h"
#include "ECS/Systems/Animation/Types.h"
#include "IO/AssetCatalog.h"
#include "IO/Loaders/SceneAssetLoader.h"
#include "IO/VFS/VFS.h"
#include "Rendering/Types/Material.h"
#include "Rendering/Types/Mesh/Mesh.h"
#include "Rendering/Types/Shader/Shader.h"
#include "Rendering/Types/Texture/Texture.h"
#include "Scenes/Scene.h"
#include "Scenes/SceneManager.h"
#include "UI/Text/Font.h"
#include "imgui.h"
#include "nlohmann/json.hpp"

#include <filesystem>
#include <set>
#include <string_view>
#include <utility>
#include <vector>

namespace {

    template <typename T> std::vector<std::string> CollectAssetKeys(const std::string_view catalogType) {
        std::set<std::string> uniqueKeys;

        if (const auto *assets = IO::AssetCatalog::GetAssets()) {
            const auto entries = assets->find(std::string(catalogType));
            if (entries != assets->end() && entries->is_array()) {
                for (const auto &entry : *entries) {
                    if (!entry.is_object()) {
                        continue;
                    }

                    const std::string id = entry.value("id", std::string{});
                    if (!id.empty()) {
                        uniqueKeys.insert(id);
                    }
                }
            }
        }

        for (const auto &[key, asset] : Core::ResourceManager::GetInstance().GetAll<T>()) {
            if (asset && !key.empty()) {
                uniqueKeys.insert(key);
            }
        }

        return {uniqueKeys.begin(), uniqueKeys.end()};
    }

    bool RetainScope(Core::EngineContext *context, IO::SceneAssetLoader::SceneAssetScope scope) {
        if (!context || !context->sceneManager) {
            return false;
        }

        auto *scene = context->sceneManager->GetCurrentScene();
        if (!scene) {
            return false;
        }

        scene->AddAssetScope(std::move(scope));
        return true;
    }

    template <typename T> bool SelectAsset(Core::EngineContext *context, const IO::SceneAssetLoader::AssetKind kind, const std::string &id, std::shared_ptr<T> &current) {
        if (!context || !context->sceneManager || !context->sceneManager->GetCurrentScene()) {
            return false;
        }

        IO::SceneAssetLoader::SceneAssetScope scope;
        if (!IO::SceneAssetLoader::Acquire(kind, id, scope)) {
            return false;
        }

        auto selected = Core::ResourceManager::GetInstance().Get<T>(id);
        if (!selected) {
            return false;
        }

        if (!RetainScope(context, std::move(scope))) {
            return false;
        }

        current = std::move(selected);
        return true;
    }

    template <typename T, typename GetPreviewText>
    bool AssetComboImpl(const char *label, Core::EngineContext *context, std::shared_ptr<T> &current, const std::string_view catalogType, const IO::SceneAssetLoader::AssetKind kind, GetPreviewText &&getPreviewText) {
        auto &resources = Core::ResourceManager::GetInstance();
        const std::vector<std::string> keys = CollectAssetKeys<T>(catalogType);

        std::string currentKey;
        if (current) {
            currentKey = resources.GetKey<T>(current);
        }

        const char *preview = current ? (currentKey.empty() ? "<Unregistered>" : currentKey.c_str()) : "None";
        bool changed = false;

        if (ImGui::BeginCombo(label, preview)) {
            const bool noneSelected = !current;
            if (ImGui::Selectable("None", noneSelected)) {
                if (current) {
                    current.reset();
                    changed = true;
                }
            }
            if (noneSelected) {
                ImGui::SetItemDefaultFocus();
            }

            for (const auto &key : keys) {
                const bool isSelected = !currentKey.empty() && key == currentKey;
                const bool isEngineBuiltin = key.starts_with("[Engine");

                if (isEngineBuiltin) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.75f, 1.0f, 1.0f));
                }

                ImGui::PushID(key.c_str());
                if (ImGui::Selectable(key.c_str(), isSelected) && !isSelected) {
                    changed = SelectAsset(context, kind, key, current);
                    if (changed) {
                        currentKey = key;
                    }
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
                ImGui::PopID();

                if (isEngineBuiltin) {
                    ImGui::PopStyleColor();
                }
            }

            ImGui::EndCombo();
        }

        if (current) {
            getPreviewText(current);
        }

        return changed;
    }

} // namespace

namespace Editor::UI {

    bool TextureCombo(const char *label, Core::EngineContext *context, std::shared_ptr<Rendering::Texture> &current) {
        return AssetComboImpl(label, context, current, "textures", IO::SceneAssetLoader::AssetKind::Texture,
                              [](const std::shared_ptr<Rendering::Texture> &texture) { ImGui::TextDisabled("%s", texture->GetPath().c_str()); });
    }

    bool ShaderCombo(const char *label, Core::EngineContext *context, std::shared_ptr<Rendering::Shader> &current) {
        return AssetComboImpl(label, context, current, "shaders", IO::SceneAssetLoader::AssetKind::Shader, [](const std::shared_ptr<Rendering::Shader> &shader) {
            ImGui::TextDisabled("Vert: %s  Frag: %s", shader->GetVertexPath().empty() ? shader->GetDebugName().c_str() : shader->GetVertexPath().c_str(),
                                shader->GetFragmentPath().empty() ? shader->GetDebugName().c_str() : shader->GetFragmentPath().c_str());
        });
    }

    bool MeshCombo(const char *label, Core::EngineContext *context, std::shared_ptr<Rendering::Mesh> &current) {
        return AssetComboImpl(label, context, current, "meshes", IO::SceneAssetLoader::AssetKind::Mesh,
                              [](const std::shared_ptr<Rendering::Mesh> &mesh) { ImGui::TextDisabled("%s, %u indices", mesh->GetFactoryId().c_str(), mesh->GetIndexCount()); });
    }

    bool MaterialCombo(const char *label, Core::EngineContext *context, std::shared_ptr<Rendering::Material> &current) {
        return AssetComboImpl(label, context, current, "materials", IO::SceneAssetLoader::AssetKind::Material, [](const std::shared_ptr<Rendering::Material> &material) {
            auto &resources = Core::ResourceManager::GetInstance();
            const std::string textureKey = material->texture ? resources.GetKey(material->texture) : "none";
            const std::string shaderKey = material->shader ? resources.GetKey(material->shader) : "none";
            ImGui::TextDisabled("Texture: %s  Shader: %s", textureKey.c_str(), shaderKey.c_str());
        });
    }

    bool FontCombo(const char *label, Core::EngineContext *context, std::shared_ptr<::UI::Font> &current) {
        const bool changed = AssetComboImpl(label, context, current, "fonts", IO::SceneAssetLoader::AssetKind::Font,
                                            [](const std::shared_ptr<::UI::Font> &font) { ImGui::TextDisabled("%upx%s", font->GetFontSize(), font->IsSDF() ? " SDF" : ""); });
        if (!current) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
            ImGui::TextWrapped("No font selected, text will not render!");
            ImGui::PopStyleColor();
        }
        return changed;
    }

    bool AnimationCombo(const char *label, Core::EngineContext *context, std::shared_ptr<const Animation::SpriteAnimationSet> &current) {
        auto mutableCurrent = std::const_pointer_cast<Animation::SpriteAnimationSet>(current);

        const bool changed = AssetComboImpl(label, context, mutableCurrent, "animation_sets", IO::SceneAssetLoader::AssetKind::AnimationSet,
                                            [](const std::shared_ptr<Animation::SpriteAnimationSet> &animation) { ImGui::TextDisabled("%zu clip(s)", animation->clips.size()); });

        if (changed) {
            current = std::move(mutableCurrent);
        }

        return changed;
    }


    bool FileCombo(const char *label, const std::string &subDir, const std::string &extension, std::string &current) {
        const auto resolved = IO::VFS::Resolve(subDir);
        std::vector<std::string> files;
        files.emplace_back("None");
        if (std::filesystem::exists(resolved)) {
            for (const auto &entry : std::filesystem::directory_iterator(resolved)) {
                if (entry.is_regular_file() && entry.path().extension() == extension) {
                    auto relPath = std::filesystem::relative(entry.path(), IO::VFS::GetProjectRoot());
                    std::string relStr = relPath.string();
                    for (auto &c : relStr) {
                        if (c == '\\') {
                            c = '/';
                        }
                    }
                    files.push_back(std::move(relStr));
                }
            }
        }

        int currentIdx = 0;
        for (int i = 1; i < static_cast<int>(files.size()); ++i) {
            if (files[i] == current) {
                currentIdx = i;
                break;
            }
        }

        const std::string preview = current.empty() ? "None" : current;
        bool changed = false;

        if (ImGui::BeginCombo(label, preview.c_str())) {
            for (int i = 0; i < static_cast<int>(files.size()); ++i) {
                const bool isSelected = currentIdx == i;
                if (ImGui::Selectable(files[i].c_str(), isSelected)) {
                    if (i == 0) {
                        current.clear();
                    } else {
                        current = files[i];
                    }
                    changed = true;
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        return changed;
    }

} // namespace Editor::UI
