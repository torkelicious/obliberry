#include "ProjectBrowserPanel.h"

#include "Applications/Editor/Platform/FileDialogs.h"
#include "Applications/Editor/States/Editor/EditState.h"
#include "Core/Constants.h"
#include "Core/ResourceManager.h"
#include "Core/Utils/OsUtils.h"
#include "Core/Utils/UiUtils.h"
#include "ECS/Systems/Animation/Animation.h"
#include "ECS/Systems/Animation/Types.h"
#include "IO/AnimationSerialization.h"
#include "IO/AssetCatalog.h"
#include "IO/Loaders/AssetLoader.h"
#include "IO/VFS/VFS.h"
#include "Logger/LoggerService.h"
#include "Platform/Threading/SmallTask.h"
#include "Rendering/Renderer.h"
#include "Rendering/Types/Material.h"
#include "Rendering/Types/Mesh/Mesh.h"
#include "Rendering/Types/Mesh/MeshFactory.h"
#include "Rendering/Types/Shader/Shader.h"
#include "Rendering/Types/Texture/Texture.h"
#include "UI/Text/Font.h"

#include <algorithm>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

#pragma push_macro("LOG_WHO")
#define LOG_WHO "ProjectBrowser"

namespace Editor::UI {

    static bool ContainsSearch(const char *haystack, const char *needle) {
        if (needle[0] == '\0')
            return true; // empty search = show all

        const std::string hay(haystack ? haystack : "");
        const std::string need(needle);
        return hay.find(need) != std::string::npos;
    }

    template <typename T> std::vector<std::pair<std::string, std::shared_ptr<T>>> GetCatalogItems(const char *catalogType) {
        auto &resources = Core::ResourceManager::GetInstance();

        std::vector<std::pair<std::string, std::shared_ptr<T>>> result;

        std::unordered_set<std::string> addedIds;

        const nlohmann::json *assets = IO::AssetCatalog::GetAssets();

        if (assets) {
            const auto type = assets->find(catalogType);

            if (type != assets->end() && type->is_array()) {
                result.reserve(type->size());

                for (const auto &definition : *type) {
                    if (!definition.is_object())
                        continue;

                    const auto idValue = definition.find("id");

                    if (idValue == definition.end() || !idValue->is_string()) {
                        continue;
                    }

                    const std::string id = idValue->get<std::string>();

                    if (id.empty() || !addedIds.insert(id).second) {
                        continue;
                    }

                    result.emplace_back(id, resources.Get<T>(id));
                }
            }
        }

        // engine assets are not stored in assets.json
        for (auto &[id, resource] : resources.GetAll<T>()) {
            const bool isEngineAsset = id.starts_with("[Engine]") || id.starts_with("[Engine_PP]");

            if (isEngineAsset && addedIds.insert(id).second) {
                result.emplace_back(id, std::move(resource));
            }
        }

        std::ranges::sort(result, {}, [](const auto &entry) -> const std::string & { return entry.first; });

        return result;
    }

    std::string ProjectBrowserPanel::KeyFromPath(const std::filesystem::path &path) { return path.stem().string(); }

    std::vector<AssetEntry> ProjectBrowserPanel::ScanDirectoryCached(const std::string &subDir, const std::string &extension) {
        const auto resolved = IO::VFS::Resolve(subDir);

        std::filesystem::file_time_type dirWrite{};
        bool dirReadable = false;
        std::error_code ec;

        if (!resolved.empty()) {
            dirWrite = std::filesystem::last_write_time(resolved, ec);

            dirReadable = !ec;
        }

        const auto it = m_ScanCaches.find(subDir);

        if (const bool haveCache = it != m_ScanCaches.end(); haveCache && it->second.valid == dirReadable && it->second.lastWrite == dirWrite) {
            return it->second.entries;
        }

        DirScanCache cache;
        cache.dirKey = subDir;
        cache.lastWrite = dirWrite;
        cache.valid = dirReadable;

        if (dirReadable)
            cache.entries = ScanDirectory(subDir, extension);

        m_ScanCaches[subDir] = std::move(cache);
        return m_ScanCaches[subDir].entries;
    }

    std::vector<AssetEntry> ProjectBrowserPanel::ScanDirectory(const std::string &subDir, const std::string &extension) {
        std::vector<AssetEntry> entries;

        const auto resolved = IO::VFS::Resolve(subDir);

        if (resolved.empty() || !std::filesystem::exists(resolved)) {
            return entries;
        }

        for (const auto &entry : std::filesystem::directory_iterator(resolved)) {
            if (!entry.is_regular_file() || entry.path().extension() != extension) {
                continue;
            }

            auto relativePath = std::filesystem::relative(entry.path(), IO::VFS::GetProjectRoot());

            std::string relative = relativePath.string();

            for (char &character : relative) {
                if (character == '\\')
                    character = '/';
            }

            entries.push_back({.name = entry.path().stem().string(), .virtualPath = relative});
        }

        return entries;
    }

    void ProjectBrowserPanel::OnImGuiRender() {
        ImGui::Begin("Project Browser");
        m_IsHovered = ImGui::IsWindowHovered();

        ImGui::InputText("##Search", m_SearchBuffer, sizeof(m_SearchBuffer));

        ImGui::SameLine();

        if (ImGui::Button("Clear"))
            m_SearchBuffer[0] = '\0';

        ImGui::SameLine();

        ImGui::RadioButton("Grid", reinterpret_cast<int *>(&m_ViewMode), static_cast<int>(ViewMode::Grid));

        ImGui::SameLine();

        ImGui::RadioButton("List", reinterpret_cast<int *>(&m_ViewMode), static_cast<int>(ViewMode::List));

        if (!m_EngineContext || !m_EngineContext->resources) {
            ImGui::TextDisabled("No project loaded.");
            ImGui::End();
            return;
        }

        auto &resources = *m_EngineContext->resources;

        if (ImGui::CollapsingHeader("Textures"))
            DrawTextureSection(resources);

        if (ImGui::CollapsingHeader("Shaders"))
            DrawShaderSection(resources);

        if (ImGui::CollapsingHeader("Meshes"))
            DrawMeshSection(resources);

        if (ImGui::CollapsingHeader("Materials"))
            DrawMaterialSection(resources);

        if (ImGui::CollapsingHeader("Fonts"))
            DrawFontSection(resources);

        if (ImGui::CollapsingHeader("Animations"))
            DrawAnimationSelection();

        if (ImGui::CollapsingHeader("Scripts")) {
            DrawFileSection("Scripts", std::string(Core::SCRIPT_PATH), std::string(Core::SCRIPT_FILE_EXTENSION), "obsl,txt", "Script Files");
        }

        if (ImGui::CollapsingHeader("Maps")) {
            DrawFileSection("Maps", std::string(Core::MAP_PATH), std::string(Core::MAP_FILE_EXTENSION), "obmap", "Map Files");
        }

        if (ImGui::CollapsingHeader("Scenes")) {
            DrawFileSection("Scenes", std::string(Core::SCENE_PATH), ".json", "json", "Scene Files");
        }

        DrawDeleteConfirmPopup(resources);
        ImGui::End();
    }

    template <typename T>
    void ProjectBrowserPanel::DrawResourceSection(Core::ResourceManager &resources, const std::vector<std::pair<std::string, std::shared_ptr<T>>> &allItems, const AssetType assetType, const char *childId,
                                                  const float childHeight, const char *emptyText, const char *typeName, std::type_identity_t<std::function<void(const std::shared_ptr<T> &)>> renderThumbnail,
                                                  const std::type_identity_t<std::function<void(const std::string &, Core::ResourceManager &)>> &renderExtraButtons,
                                                  std::type_identity_t<std::function<void(const std::string &, const std::shared_ptr<T> &, Core::ResourceManager &)>> renderTooltip) {
        struct RenameOp {
            std::string oldKey;
            std::string newKey;
        };

        std::vector<RenameOp> pendingRenames;

        if (ImGui::BeginChild(childId, ImVec2(0, childHeight), true)) {
            if (m_ViewMode == ViewMode::Grid) {
                int itemsPerRow = static_cast<int>(ImGui::GetContentRegionAvail().x / 180.0f);
                if (itemsPerRow < 1)
                    itemsPerRow = 1;

                int itemCount = 0;
                for (const auto &[id, asset] : allItems) {
                    if (!ContainsSearch(id.c_str(), m_SearchBuffer)) {
                        continue;
                    }

                    if (itemCount % itemsPerRow != 0)
                        ImGui::SameLine();

                    ImGui::PushID(id.c_str());
                    ImGui::BeginGroup();

                    renderThumbnail(asset);

                    ImGui::SameLine();
                    ImGui::BeginGroup();
                    ImGui::Text("%s", id.c_str());

                    if (m_RenamingKey == id) {
                        if (m_RenameJustActivated) {
                            ImGui::SetKeyboardFocusHere();
                            m_RenameJustActivated = false;
                        }
                        ImGui::SetNextItemWidth(80.0f);
                        ImGui::InputText("##rename", m_RenameBuffer, sizeof(m_RenameBuffer));
                        if (ImGui::SmallButton("Save")) {
                            if (m_RenameBuffer[0] != '\0' && !resources.Get<T>(m_RenameBuffer)) {
                                pendingRenames.push_back({id, m_RenameBuffer});
                            }
                            m_RenamingKey.clear();
                        }
                        ImGui::SameLine();

                        if (ImGui::SmallButton("Cancel"))
                            m_RenamingKey.clear();
                    } else {
                        const bool isEngineBuiltin = !id.empty() && id[0] == '[';
                        if (!isEngineBuiltin) {
                            if (ImGui::SmallButton("Rename")) {
                                m_RenamingKey = id;
                                m_RenameJustActivated = true;
                                strncpy(m_RenameBuffer, id.c_str(), sizeof(m_RenameBuffer));
                                m_RenameBuffer[sizeof(m_RenameBuffer) - 1] = '\0';
                            }
                        }
                        renderExtraButtons(id, resources);

                        if (!isEngineBuiltin && ImGui::SmallButton("Remove")) {
                            m_DeleteConfirmKey = id;
                            m_DeleteConfirmType = assetType;
                        }
                    }

                    ImGui::EndGroup();
                    ImGui::EndGroup();

                    auto *drawList = ImGui::GetWindowDrawList();

                    const auto minimum = ImGui::GetItemRectMin();

                    const auto maximum = ImGui::GetItemRectMax();

                    drawList->AddRect(ImVec2(minimum.x - 2, minimum.y - 2), ImVec2(maximum.x + 2, maximum.y + 2), ImGui::GetColorU32(ImGuiCol_Border));

                    ImGui::PopID();
                    ++itemCount;
                }
            } else {
                for (const auto &[id, asset] : allItems) {
                    if (!ContainsSearch(id.c_str(), m_SearchBuffer)) {
                        continue;
                    }
                    ImGui::PushID(id.c_str());

                    if (m_RenamingKey == id) {
                        if (m_RenameJustActivated) {
                            ImGui::SetKeyboardFocusHere();
                            m_RenameJustActivated = false;
                        }
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 100.0f);
                        ImGui::InputText("##rename", m_RenameBuffer, sizeof(m_RenameBuffer));
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Save")) {
                            if (m_RenameBuffer[0] != '\0' && !resources.Get<T>(m_RenameBuffer)) {
                                pendingRenames.push_back({id, m_RenameBuffer});
                            }
                            m_RenamingKey.clear();
                        }
                        ImGui::SameLine();

                        if (ImGui::SmallButton("Cancel"))
                            m_RenamingKey.clear();
                    } else {
                        const bool isEngineBuiltin = !id.empty() && id[0] == '[';
                        ImGui::Text("%s", id.c_str());
                        if (renderTooltip && ImGui::IsItemHovered()) {
                            renderTooltip(id, asset, resources);
                        }
                        ImGui::SameLine();
                        if (!isEngineBuiltin) {
                            if (ImGui::SmallButton("Rename")) {
                                m_RenamingKey = id;
                                m_RenameJustActivated = true;
                                strncpy(m_RenameBuffer, id.c_str(), sizeof(m_RenameBuffer));
                                m_RenameBuffer[sizeof(m_RenameBuffer) - 1] = '\0';
                            }
                            ImGui::SameLine();
                        }
                        renderExtraButtons(id, resources);
                        if (!isEngineBuiltin) {
                            ImGui::SameLine();
                            if (ImGui::SmallButton("Remove")) {
                                m_DeleteConfirmKey = id;
                                m_DeleteConfirmType = assetType;
                            }
                        }
                    }

                    ImGui::PopID();
                }
            }
        }
        ImGui::EndChild();

        for (const auto &[oldKey, newKey] : pendingRenames) {
            auto resource = resources.Get<T>(oldKey);

            resources.Unload<T>(oldKey);

            resources.LoadFromFactory<T>(newKey, [resource] { return resource; });

            LOG_INFO(LOG_WHO, std::string("Renamed ") + typeName + " '" + oldKey + "' -> '" + newKey + "'");
        }

        if (allItems.empty() || (m_SearchBuffer[0] != '\0' && pendingRenames.empty())) {
            ImGui::TextDisabled("%s", emptyText);
        }
    }

    void ProjectBrowserPanel::DrawTextureSection(Core::ResourceManager &resources) {
        const auto allTextures = GetCatalogItems<Rendering::Texture>("textures");

        if (m_SearchBuffer[0] != '\0') {
            ImGui::SameLine();

            if (ImGui::SmallButton("Clear"))
                m_SearchBuffer[0] = '\0';
        }

        DrawResourceSection(
                resources, allTextures, AssetType::Texture, "##texList", 130.0f, "No textures imported.", "texture",
                [](const std::shared_ptr<Rendering::Texture> &texture) {
                    if (texture) {
                        Core::Utils::UI::ImGuiImageFlipped(texture->GetID(), ImVec2(64, 64));
                    } else {
                        ImGui::Button("T", ImVec2(64, 64));
                    }
                },
                [this](const std::string &id, Core::ResourceManager &resourceManager) {
                    if (ImGui::SmallButton("Replace"))
                        ReplaceTexture(resourceManager, id);
                });

        ImGui::Spacing();

        if (ImGui::SmallButton("Import Texture"))
            ImportTexture(resources);
    }

    void ProjectBrowserPanel::DrawShaderSection(Core::ResourceManager &resources) {
        const auto allShaders = GetCatalogItems<Rendering::Shader>("shaders");

        DrawResourceSection(
                resources, allShaders, AssetType::Shader, "##shaderList", 130.0f, "No shaders imported.", "shader", [](const std::shared_ptr<Rendering::Shader> &) { ImGui::Button("S", ImVec2(64, 64)); },
                [this](const std::string &id, Core::ResourceManager &resourceManager) {
                    if (ImGui::SmallButton("Replace"))
                        ReplaceShader(resourceManager, id);
                });

        ImGui::Spacing();

        if (ImGui::SmallButton("Import Shader"))
            ImportShader(resources);
    }

    void ProjectBrowserPanel::DrawMeshSection(Core::ResourceManager &resources) {
        const auto allMeshes = GetCatalogItems<Rendering::Mesh>("meshes");

        DrawResourceSection(
                resources, allMeshes, AssetType::Mesh, "##meshList", 100.0f, "No meshes registered.", "mesh", [](const std::shared_ptr<Rendering::Mesh> &) { ImGui::Button("M", ImVec2(64, 64)); },
                [](const std::string &, Core::ResourceManager &) {});

        ImGui::Spacing();
        ImGui::SeparatorText("Create Mesh");

        constexpr const char *meshTypes[] = {"Quad", "PointTopHex", "ETriang", "Ellipse", "Circle", "Pentagon", "Hexagon", "Octagon", "Ring", "Sector", "Diamond", "Custom"};
        ImGui::Combo("Type", &m_SelectedMeshFactory, meshTypes, IM_ARRAYSIZE(meshTypes));
        ImGui::InputText("ID##mesh", m_MeshNameBuffer, sizeof(m_MeshNameBuffer));

        if (ImGui::Button("Create Mesh")) {
            if (m_MeshNameBuffer[0] == '\0') {
                snprintf(m_MeshNameBuffer, sizeof(m_MeshNameBuffer), "%s_mesh", meshTypes[m_SelectedMeshFactory]);
            }

            const std::string id(m_MeshNameBuffer);

            if (resources.Get<Rendering::Mesh>(id)) {
                LOG_ERROR(LOG_WHO, "Mesh '" + id + "' already exists");
            } else {
                CreateMesh(resources);
            }
        }
    }

    void ProjectBrowserPanel::DrawMaterialSection(Core::ResourceManager &resources) {
        const auto allMaterials = GetCatalogItems<Rendering::Material>("materials");

        DrawResourceSection(
                resources, allMaterials, AssetType::Material, "##matList", 100.0f, "No materials registered.", "material",
                [](const std::shared_ptr<Rendering::Material> &material) {
                    if (material && material->texture) {
                        Core::Utils::UI::ImGuiImageFlipped(material->texture->GetID(), ImVec2(64, 64));
                    } else {
                        ImGui::Button("M", ImVec2(64, 64));
                    }
                },
                [](const std::string &, Core::ResourceManager &) {},
                [](const std::string &id, const std::shared_ptr<Rendering::Material> &material, Core::ResourceManager &resourceManager) {
                    if (!material) {
                        ImGui::SetTooltip("%s\nNot currently loaded", id.c_str());
                        return;
                    }

                    const std::string shaderId = material->shader ? resourceManager.GetKey(material->shader) : "none";

                    const std::string textureId = material->texture ? resourceManager.GetKey(material->texture) : "none";

                    ImGui::SetTooltip("Shader: %s\nTexture: %s", shaderId.c_str(), textureId.c_str());
                });

        ImGui::Spacing();
        ImGui::SeparatorText("Create Material");

        const auto allShaders = GetCatalogItems<Rendering::Shader>("shaders");

        std::vector<std::string> shaderKeys{"None"};

        for (const auto &[key, unused] : allShaders) {
            (void)unused;
            shaderKeys.push_back(key);
        }

        if (m_SelectedMaterialShaderIdx >= static_cast<int>(shaderKeys.size())) {
            m_SelectedMaterialShaderIdx = 0;
        }
        ImGui::Combo(
                "Shader##createMat", &m_SelectedMaterialShaderIdx,
                [](void *data, const int index) -> const char * {
                    const auto &keys = *static_cast<std::vector<std::string> *>(data);

                    if (index < 0 || index >= static_cast<int>(keys.size())) {
                        return "";
                    }

                    return keys[index].c_str();
                },
                &shaderKeys, static_cast<int>(shaderKeys.size()));

        // Texture
        const auto allTextures = GetCatalogItems<Rendering::Texture>("textures");

        std::vector<std::string> textureKeys{"None"};

        for (const auto &[key, unused] : allTextures) {
            (void)unused;
            textureKeys.push_back(key);
        }

        if (m_SelectedMaterialTextureIdx >= static_cast<int>(textureKeys.size())) {
            m_SelectedMaterialTextureIdx = 0;
        }
        ImGui::Combo(
                "Texture##createMat", &m_SelectedMaterialTextureIdx,
                [](void *data, const int index) -> const char * {
                    const auto &keys = *static_cast<std::vector<std::string> *>(data);

                    if (index < 0 || index >= static_cast<int>(keys.size())) {
                        return "";
                    }

                    return keys[index].c_str();
                },
                &textureKeys, static_cast<int>(textureKeys.size()));

        ImGui::ColorEdit4("Color##createMat", &m_MaterialColor.x, ImGuiColorEditFlags_NoInputs);
        ImGui::InputText("ID##mat", m_MaterialNameBuffer, sizeof(m_MaterialNameBuffer));

        if (ImGui::Button("Create Material")) {
            if (m_MaterialNameBuffer[0] == '\0') {
                LOG_ERROR(LOG_WHO, "Material name cannot be empty");
                return;
            }

            const std::string id(m_MaterialNameBuffer);

            if (resources.Get<Rendering::Material>(id)) {
                LOG_ERROR(LOG_WHO, "Material '" + id + "' already exists");
                return;
            }

            std::shared_ptr<Rendering::Shader> shader;

            if (m_SelectedMaterialShaderIdx > 0 && m_SelectedMaterialShaderIdx < static_cast<int>(shaderKeys.size())) {
                shader = resources.Get<Rendering::Shader>(shaderKeys[m_SelectedMaterialShaderIdx]);
            }

            std::shared_ptr<Rendering::Texture> texture;

            if (m_SelectedMaterialTextureIdx > 0 && m_SelectedMaterialTextureIdx < static_cast<int>(textureKeys.size())) {
                texture = resources.Get<Rendering::Texture>(textureKeys[m_SelectedMaterialTextureIdx]);
            }

            resources.LoadFromFactory<Rendering::Material>(
                    id, [shader, texture, color = m_MaterialColor] { return std::make_shared<Rendering::Material>(Rendering::Material{.shader = shader, .texture = texture, .color = color}); });

            LOG_INFO(LOG_WHO, "Created material '" + id + "'");

            m_MaterialNameBuffer[0] = '\0';
            m_MaterialColor = {1.0f, 1.0f, 1.0f, 1.0f};

            m_SelectedMaterialShaderIdx = 0;
            m_SelectedMaterialTextureIdx = 0;
        }
    }

    void ProjectBrowserPanel::DrawFontSection(Core::ResourceManager &resources) {
        const auto allFonts = GetCatalogItems<::UI::Font>("fonts");

        DrawResourceSection(
                resources, allFonts, AssetType::Font, "##fontList", 100.0f, "No fonts imported.", "font",
                [](const std::shared_ptr<::UI::Font> &font) {
                    ImGui::Text("%s", font ? "F" : "?");
                    if (font) {
                        ImGui::SameLine();
                        ImGui::TextDisabled("%upx%s", font->GetFontSize(), font->IsSDF() ? " SDF" : "");
                    }
                },
                [this](const std::string &id, Core::ResourceManager &resourceManager) {
                    if (ImGui::SmallButton("Replace"))
                        ReplaceFont(resourceManager, id);
                },
                [](const std::string &key, const std::shared_ptr<::UI::Font> &font, Core::ResourceManager &) {
                    if (font) {
                        ImGui::SetTooltip("%s\nSize: %u\nSDF: %s"
                                          "\nGlyphs: loaded",
                                          key.c_str(), font->GetFontSize(), font->IsSDF() ? "yes" : "no");
                    } else {
                        ImGui::SetTooltip("%s\nNot currently loaded", key.c_str());
                    }
                });

        ImGui::Spacing();
        ImGui::SeparatorText("Import Font");

        ImGui::InputInt("Size", &m_FontSize);
        if (m_FontSize < 1)
            m_FontSize = 1;
        ImGui::Checkbox("SDF", &m_FontUseSDF);
        if (m_FontUseSDF) {
            ImGui::SameLine();
            ImGui::InputInt("Spread", &m_FontSDFSpread);
            if (m_FontSDFSpread < 1)
                m_FontSDFSpread = 1;
        }

        if (ImGui::SmallButton("Import Font"))
            ImportFont(resources);
    }

    void ProjectBrowserPanel::ImportFont(Core::ResourceManager &resources) const {
        if (!m_EngineContext)
            return;

        const auto picked = Platform::FileDialogs::OpenFile(*m_EngineContext, {.filterName = "Font", .filterExt = "ttf,otf"});

        if (!picked)
            return;

        const auto finalPath = IO::AssetLoader::ImportAsset(*picked, "fonts");

        if (!finalPath)
            return;

        const std::string key = KeyFromPath(std::filesystem::path(*finalPath));

        if (resources.Get<::UI::Font>(key)) {
            LOG_WARN(LOG_WHO, "Font '" + key + "' already exists. Skipping");
            return;
        }

        auto font = resources.Load<::UI::Font>(key, *finalPath, static_cast<unsigned int>(m_FontSize), m_FontUseSDF, static_cast<unsigned int>(m_FontSDFSpread));

        std::thread([font] {
            font->LoadCPU();
            Rendering::Renderer::SubmitInitTask(::Platform::Threading::SmallTask([font] { font->InitGL(); }));
        }).detach();

        LOG_INFO(LOG_WHO, "Imported font '" + key + "' from " + *finalPath);
    }

    void ProjectBrowserPanel::ReplaceFont(Core::ResourceManager &resources, const std::string &key) const {
        if (!m_EngineContext)
            return;

        const auto picked = Platform::FileDialogs::OpenFile(*m_EngineContext, {.filterName = "Font", .filterExt = "ttf,otf"});

        if (!picked)
            return;

        const auto finalPath = IO::AssetLoader::ImportAsset(*picked, "fonts");

        if (!finalPath)
            return;

        resources.Unload<::UI::Font>(key);

        auto font = resources.Load<::UI::Font>(key, *finalPath, static_cast<unsigned int>(m_FontSize), m_FontUseSDF, static_cast<unsigned int>(m_FontSDFSpread));

        std::thread([font] {
            font->LoadCPU();
            Rendering::Renderer::SubmitInitTask(::Platform::Threading::SmallTask([font] { font->InitGL(); }));
        }).detach();
        LOG_INFO(LOG_WHO, "Replaced font '" + key + "'");
    }

} // namespace Editor::UI

void Editor::UI::ProjectBrowserPanel::DrawFileSection(const char *label, const std::string &directory, const std::string &extension, const char *importFilter, const char *importFilterName) {
    const bool isScripts = std::strcmp(label, "Scripts") == 0;

    auto entries = ScanDirectoryCached(directory, extension);

    ImGui::PushID(label);
    if (ImGui::BeginChild("list", ImVec2(0, 120), true)) {
        if (m_ViewMode == ViewMode::Grid) {
            int itemsPerRow = static_cast<int>(ImGui::GetContentRegionAvail().x / 180.0f);
            if (itemsPerRow < 1)
                itemsPerRow = 1;

            int itemCount = 0;

            for (const auto &[name, virtualPath] : entries) {
                (void)name;

                if (!ContainsSearch(virtualPath.c_str(), m_SearchBuffer)) {
                    continue;
                }

                if (itemCount % itemsPerRow != 0)
                    ImGui::SameLine();

                ImGui::PushID(virtualPath.c_str());
                ImGui::BeginGroup();

                ImGui::Button("F", ImVec2(64, 64));

                ImGui::SameLine();
                ImGui::BeginGroup();
                ImGui::Text("%s", virtualPath.c_str());

                if (isScripts && ImGui::SmallButton("Edit")) {
                    Core::Utils::OS::OsOpenFile(IO::VFS::Resolve(virtualPath));
                }

                if (ImGui::SmallButton("Remove")) {
                    m_DeleteConfirmKey = virtualPath;
                    m_DeleteConfirmFilePath = virtualPath;

                    m_DeleteConfirmType = AssetType::Material;
                }

                ImGui::EndGroup();
                ImGui::EndGroup();

                auto *drawList = ImGui::GetWindowDrawList();

                const auto minimum = ImGui::GetItemRectMin();

                const auto maximum = ImGui::GetItemRectMax();

                drawList->AddRect(ImVec2(minimum.x - 2, minimum.y - 2), ImVec2(maximum.x + 2, maximum.y + 2), ImGui::GetColorU32(ImGuiCol_Border));

                ImGui::PopID();
                ++itemCount;
            }
        } else {
            for (const auto &[name, virtualPath] : entries) {
                (void)name;

                if (!ContainsSearch(virtualPath.c_str(), m_SearchBuffer)) {
                    continue;
                }

                ImGui::PushID(virtualPath.c_str());
                ImGui::Text("%s", virtualPath.c_str());
                ImGui::SameLine();

                if (isScripts && ImGui::SmallButton("Edit")) {
                    Core::Utils::OS::OsOpenFile(IO::VFS::Resolve(virtualPath));
                }

                if (ImGui::SmallButton("Remove")) {
                    m_DeleteConfirmKey = virtualPath;
                    m_DeleteConfirmFilePath = virtualPath;

                    m_DeleteConfirmType = AssetType::Material;
                }

                ImGui::PopID();
            }
        }
    }
    ImGui::EndChild();
    ImGui::PopID(); // label

    if (entries.empty() || (m_SearchBuffer[0] != '\0' && std::ranges::all_of(entries, [this](const AssetEntry &entry) { return !ContainsSearch(entry.virtualPath.c_str(), m_SearchBuffer); }))) {
        ImGui::TextDisabled("No %s found.", importFilterName);
    }

    ImGui::Spacing();
    if (ImGui::SmallButton(("Import " + std::string(importFilterName)).c_str())) {
        ImportFile(directory, importFilter, importFilterName);
    }

    if (isScripts) {
        ImGui::SameLine();

        ImGui::InputText("##newScript", m_NewScriptBuffer, sizeof(m_NewScriptBuffer));

        ImGui::SameLine();
        if (ImGui::SmallButton("New Script")) {
            if (m_NewScriptBuffer[0] != '\0') {
                std::string filename(m_NewScriptBuffer);

                if (!filename.ends_with(".obsl"))
                    filename += ".obsl";

                const auto directoryPath = IO::VFS::Resolve(std::string(Core::SCRIPT_PATH));

                if (!directoryPath.empty()) {
                    std::filesystem::create_directories(directoryPath);

                    const auto filePath = directoryPath / filename;

                    if (!std::filesystem::exists(filePath)) {
                        std::ofstream file(filePath);
                        file << "// " << m_NewScriptBuffer << '\n';

                        LOG_INFO(LOG_WHO, "Created script '" + filename + "'");
                    } else {
                        LOG_ERROR(LOG_WHO, "Script '" + filename + "' already exists");
                    }
                }

                m_NewScriptBuffer[0] = '\0';
            }
        }
    }
}

void Editor::UI::ProjectBrowserPanel::DrawDeleteConfirmPopup(Core::ResourceManager &resources) {
    if (m_DeleteConfirmKey.empty())
        return;

    ImGui::OpenPopup("Confirm Remove");
    if (ImGui::BeginPopupModal("Confirm Remove", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Are you sure you want to remove '%s'?", m_DeleteConfirmKey.c_str());
        if (!m_DeleteConfirmFilePath.empty()) {
            ImGui::TextDisabled("This will permanently delete "
                                "the file from disk.");
        }
        ImGui::Separator();

        if (ImGui::Button("Yes, Remove", ImVec2(120, 0))) {
            if (!m_DeleteConfirmFilePath.empty()) {
                const auto absolutePath = IO::VFS::Resolve(m_DeleteConfirmFilePath);

                if (!absolutePath.empty() && std::filesystem::remove(absolutePath)) {
                    LOG_INFO(LOG_WHO, "Deleted file '" + m_DeleteConfirmFilePath + "'");
                } else {
                    LOG_ERROR(LOG_WHO, "Failed to delete '" + m_DeleteConfirmFilePath + "'");
                }
                // RM (as in resource manager, not rm) asset
            } else {
                switch (m_DeleteConfirmType) {
                    case AssetType::Texture:
                        resources.Unload<Rendering::Texture>(m_DeleteConfirmKey);
                        break;
                    case AssetType::Shader:
                        resources.Unload<Rendering::Shader>(m_DeleteConfirmKey);
                        break;
                    case AssetType::Mesh:
                        resources.Unload<Rendering::Mesh>(m_DeleteConfirmKey);
                        break;
                    case AssetType::Material:
                        resources.Unload<Rendering::Material>(m_DeleteConfirmKey);
                        break;
                    case AssetType::Font:
                        resources.Unload<::UI::Font>(m_DeleteConfirmKey);
                        break;
                    case AssetType::Animation:
                        resources.Unload<Animation::SpriteAnimationSet>(m_DeleteConfirmKey);
                        break;
                }
                LOG_INFO(LOG_WHO, "Removed asset '" + m_DeleteConfirmKey + "'");
            }
            ImGui::CloseCurrentPopup();
            m_DeleteConfirmKey.clear();
            m_DeleteConfirmFilePath.clear();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            m_DeleteConfirmKey.clear();
            m_DeleteConfirmFilePath.clear();
        }
        ImGui::EndPopup();
    }
}

void Editor::UI::ProjectBrowserPanel::ImportTexture(Core::ResourceManager &resources) const {
    if (!m_EngineContext)
        return;

    const auto picked = Platform::FileDialogs::OpenFile(*m_EngineContext, {.filterName = "Image", .filterExt = "png,jpg,jpeg,bmp,tga"});

    if (!picked)
        return;

    const auto finalPath = IO::AssetLoader::ImportAsset(*picked, "textures");

    if (!finalPath)
        return;

    const std::string key = KeyFromPath(std::filesystem::path(*finalPath));

    if (resources.Get<Rendering::Texture>(key)) {
        LOG_WARN(LOG_WHO, "Texture '" + key + "' already exists. Skipping");
        return;
    }

    auto texture = resources.Load<Rendering::Texture>(key, *finalPath);

    Rendering::Renderer::SubmitInitTask(::Platform::Threading::SmallTask([texture] { texture->InitGL(); }));

    LOG_INFO(LOG_WHO, "Imported texture '" + key + "' from " + *finalPath);
}

void Editor::UI::ProjectBrowserPanel::ImportShader(Core::ResourceManager &resources) const {
    if (!m_EngineContext)
        return;

    const auto vertexPicked = Platform::FileDialogs::OpenFile(*m_EngineContext, {.filterName = "Vertex Shader", .filterExt = "vert,glsl"});

    if (!vertexPicked)
        return;

    const auto fragmentPicked = Platform::FileDialogs::OpenFile(*m_EngineContext, {.filterName = "Fragment Shader", .filterExt = "frag,glsl"});

    if (!fragmentPicked)
        return;

    const auto finalVertex = IO::AssetLoader::ImportAsset(*vertexPicked, "shaders");

    const auto finalFragment = IO::AssetLoader::ImportAsset(*fragmentPicked, "shaders");

    if (!finalVertex || !finalFragment)
        return;

    const std::string key = KeyFromPath(std::filesystem::path(*finalVertex));

    if (resources.Get<Rendering::Shader>(key)) {
        LOG_WARN(LOG_WHO, "Shader '" + key + "' already exists. Skipping");
        return;
    }

    auto shader = resources.Load<Rendering::Shader>(key, *finalVertex, *finalFragment);

    Rendering::Renderer::SubmitInitTask(::Platform::Threading::SmallTask([shader] { shader->InitGL(); }));
    LOG_INFO(LOG_WHO, "Imported shader '" + key + "'");
}

void Editor::UI::ProjectBrowserPanel::ReplaceTexture(Core::ResourceManager &resources, const std::string &key) const {
    if (!m_EngineContext)
        return;

    const auto picked = Platform::FileDialogs::OpenFile(*m_EngineContext, {.filterName = "Image", .filterExt = "png,jpg,jpeg,bmp,tga"});

    if (!picked)
        return;

    const auto finalPath = IO::AssetLoader::ImportAsset(*picked, "textures");

    if (!finalPath)
        return;

    resources.Unload<Rendering::Texture>(key);

    auto texture = resources.Load<Rendering::Texture>(key, *finalPath);

    Rendering::Renderer::SubmitInitTask(::Platform::Threading::SmallTask([texture] { texture->InitGL(); }));

    LOG_INFO(LOG_WHO, "Replaced texture '" + key + "'");
}

void Editor::UI::ProjectBrowserPanel::ReplaceShader(Core::ResourceManager &resources, const std::string &key) const {
    if (!m_EngineContext)
        return;

    const auto vertexPicked = Platform::FileDialogs::OpenFile(*m_EngineContext, {.filterName = "Vertex Shader", .filterExt = "vert,glsl"});

    if (!vertexPicked)
        return;

    const auto fragmentPicked = Platform::FileDialogs::OpenFile(*m_EngineContext, {.filterName = "Fragment Shader", .filterExt = "frag,glsl"});

    if (!fragmentPicked)
        return;

    const auto finalVertex = IO::AssetLoader::ImportAsset(*vertexPicked, "shaders");

    const auto finalFragment = IO::AssetLoader::ImportAsset(*fragmentPicked, "shaders");

    if (!finalVertex || !finalFragment)
        return;

    resources.Unload<Rendering::Shader>(key);

    auto shader = resources.Load<Rendering::Shader>(key, *finalVertex, *finalFragment);

    Rendering::Renderer::SubmitInitTask(::Platform::Threading::SmallTask([shader] { shader->InitGL(); }));
    LOG_INFO(LOG_WHO, "Replaced shader '" + key + "'");
}

void Editor::UI::ProjectBrowserPanel::CreateMesh(Core::ResourceManager &resources) {
    const std::string id(m_MeshNameBuffer);
    if (id.empty())
        return;

    Rendering::MeshData data;
    constexpr const char *meshTypes[] = {"Quad", "PointTopHex", "ETriang", "Ellipse", "Circle", "Pentagon", "Hexagon", "Octagon", "Ring", "Sector", "Diamond", "Custom"};
    switch (m_SelectedMeshFactory) {
        case 0:
            data = Rendering::MeshFactory::CreateQuad();
            break;
        case 1:
            data = Rendering::MeshFactory::CreatePointTopHex();
            break;
        case 2:
            data = Rendering::MeshFactory::CreateEquiTriangle(0.5f);
            break;
        case 3:
            data = Rendering::MeshFactory::CreateEllipse();
            break;
        case 4:
            data = Rendering::MeshFactory::CreateRegularPolygon(32);
            break;
        case 5:
            data = Rendering::MeshFactory::CreateRegularPolygon(5);
            break;
        case 6:
            data = Rendering::MeshFactory::CreateRegularPolygon(6);
            break;
        case 7:
            data = Rendering::MeshFactory::CreateRegularPolygon(8);
            break;
        case 8:
            data = Rendering::MeshFactory::CreateRing();
            break;
        case 9:
            data = Rendering::MeshFactory::CreateSector();
            break;
        case 10:
            data = Rendering::MeshFactory::CreateDiamond();
            break;
        case 11:
            States::EditState::ShowMeshCreator();
            return;
        default:
            return;
    }

    auto mesh = resources.LoadFromFactory<Rendering::Mesh>(id, [data = std::move(data), meshTypes, factoryIndex = m_SelectedMeshFactory] {
        auto result = std::make_shared<Rendering::Mesh>(data);

        result->SetFactoryId(meshTypes[factoryIndex]);

        return result;
    });

    Rendering::Renderer::SubmitInitTask(::Platform::Threading::SmallTask([mesh] { mesh->InitGL(); }));
    m_MeshNameBuffer[0] = '\0';
    LOG_INFO(LOG_WHO, "Created mesh '" + id + "'");
}

void Editor::UI::ProjectBrowserPanel::ImportFile(const std::string &targetSubDir, const char *filterExt, const char *filterName) const {
    if (!m_EngineContext)
        return;

    const auto picked = Platform::FileDialogs::OpenFile(*m_EngineContext, {.filterName = filterName, .filterExt = filterExt});

    if (!picked)
        return;

    const auto directory = IO::VFS::Resolve(targetSubDir);

    if (directory.empty())
        return;

    std::filesystem::create_directories(directory);

    std::filesystem::copy(*picked, directory / std::filesystem::path(*picked).filename(), std::filesystem::copy_options::overwrite_existing);
}

void Editor::UI::ProjectBrowserPanel::ImportAnimation() const {
    auto &resources = Core::ResourceManager::GetInstance();

    if (!m_EngineContext)
        return;

    const auto picked = Platform::FileDialogs::OpenFile(*m_EngineContext, {.filterName = "Sprite Animation", .filterExt = "json"});

    if (!picked)
        return;

    const std::string key = KeyFromPath(*picked);

    if (resources.Get<Animation::SpriteAnimationSet>(key)) {
        LOG_WARN(LOG_WHO, "Animation '" + key + "' already exists. Skipping");
        return;
    }

    const auto finalPath = IO::AssetLoader::ImportAsset(*picked, "animations");

    if (!finalPath)
        return;

    try {
        auto animation = IO::AnimationIO::Deserialize(*finalPath);

        if (!animation.sheet || !Animation::ValidateSet(animation)) {
            LOG_ERROR(LOG_WHO, "Could not import animation '" + key + "'");
            return;
        }

        animation.path = *finalPath;

        resources.Register(key, std::make_shared<Animation::SpriteAnimationSet>(std::move(animation)));

        LOG_INFO(LOG_WHO, "Imported animation '" + key + "' from " + *finalPath);
    } catch (const std::exception &exception) {
        LOG_ERROR(LOG_WHO, "Could not import animation '" + key + "': " + exception.what());
    }
}

void Editor::UI::ProjectBrowserPanel::DrawAnimationSelection() {
    auto &resources = Core::ResourceManager::GetInstance();

    const auto animations = GetCatalogItems<Animation::SpriteAnimationSet>("animation_sets");

    DrawResourceSection(
            resources, animations, AssetType::Animation, "##animationList", 130.0f, "No animations imported.", "animation",
            [](const std::shared_ptr<Animation::SpriteAnimationSet> &animation) {
                if (animation && animation->sheet && animation->sheet->texture) {
                    Core::Utils::UI::ImGuiImageFlipped(animation->sheet->texture->GetID(), ImVec2(64, 64));
                } else {
                    ImGui::Button("animation", ImVec2(64, 64));
                }
            },
            [this](const std::string &key, Core::ResourceManager &resourceManager) {
                ImGui::BeginDisabled(!OnEditAnimation);
                if (ImGui::SmallButton("Edit")) {
                    const auto animation = resourceManager.Get<Animation::SpriteAnimationSet>(key);

                    if (animation && OnEditAnimation)
                        OnEditAnimation(key, animation);
                }

                ImGui::EndDisabled();
            });

    if (ImGui::Button("Import Animation"))
        ImportAnimation();

    ImGui::SameLine();
    ImGui::BeginDisabled(!OnCreateAnimation);
    if (ImGui::Button("New Animation")) {
        m_NewAnimationID[0] = '\0';
        m_CreateAnimationError.clear();
        ImGui::OpenPopup("Create Animation");
    }
    ImGui::EndDisabled();
    DrawCreateAnimation();
}

void Editor::UI::ProjectBrowserPanel::DrawCreateAnimation() {
    if (!ImGui::BeginPopupModal("Create Animation", nullptr, ImGuiChildFlags_AlwaysAutoResize)) {
        return;
    }

    ImGui::InputText("Resource ID (name)", m_NewAnimationID, sizeof(m_NewAnimationID));

    if (!m_CreateAnimationError.empty()) {
        ImGui::TextWrapped("%s", m_CreateAnimationError.c_str());
    }

    if (ImGui::Button("Choose & Create")) {
        const auto create = [this] {
            const std::string key = m_NewAnimationID;
            auto &resources = Core::ResourceManager::GetInstance();

            if (key.find_first_not_of(" \t\r\n") == std::string::npos) {
                m_CreateAnimationError = "Enter an ID.";
                return;
            }

            if (resources.Get<Animation::SpriteAnimationSet>(key)) {
                m_CreateAnimationError = "Resource ID is already taken!";
                return;
            }

            if (!m_EngineContext || !OnCreateAnimation) {
                m_CreateAnimationError = "Animation editor is unavailable.";
                return;
            }

            const auto directory = IO::VFS::GetAssetsDirectory() / "animations";

            std::error_code error;

            std::filesystem::create_directories(directory, error);

            if (error) {
                m_CreateAnimationError = "Could not create asset "
                                         "directory: " +
                                         error.message();
                return;
            }

            const std::string defaultPath = directory.string();

            const std::string defaultName = key + ".json";

            const auto picked = Platform::FileDialogs::SaveFile(*m_EngineContext, {.filterName = "Sprite Animation", .filterExt = "json", .defaultPath = defaultPath.c_str(), .defaultName = defaultName.c_str()});

            if (!picked)
                return;

            std::filesystem::path path = *picked;

            if (path.extension() != ".json")
                path += ".json";

            const bool exists = std::filesystem::exists(path, error);

            if (error) {
                m_CreateAnimationError = "I/O error: " + error.message();
                return;
            }

            if (exists) {
                m_CreateAnimationError = "File already exists.";
                return;
            }

            std::filesystem::path virtualPath;

            try {
                virtualPath = IO::VFS::ToRelative(path);
            } catch (const std::filesystem::filesystem_error &exception) {
                m_CreateAnimationError = exception.what();
                return;
            }

            if (virtualPath.empty() || virtualPath.is_absolute()) {
                m_CreateAnimationError = "Location is not inside "
                                         "the project.";
                return;
            }

            for (const auto &part : virtualPath) {
                if (part == "..") {
                    m_CreateAnimationError = "Choose a location inside "
                                             "the project.";
                    return;
                }
            }

            OnCreateAnimation(key, virtualPath);
            m_CreateAnimationError.clear();
            ImGui::CloseCurrentPopup();
        };
        create();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
        ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
}

#pragma pop_macro("LOG_WHO")
