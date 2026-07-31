#include "ProjectBrowserPanel.h"
#include "Platform/Threading/SmallTask.h"
#include <thread>

#include "Core/Constants.h"
#include "Logger/LoggerService.h"
#include "Applications/Editor/Platform/FileDialogs.h"
#include "Core/Utils/UiUtils.h"
#include "IO/VFS/VFS.h"
#include "IO/Loaders/AssetLoader.h"
#include "Rendering/Material.h"
#include "Rendering/Mesh.h"
#include "Rendering/MeshFactory.h"
#include "Rendering/Renderer.h"
#include "Rendering/Texture.h"
#include "Rendering/Shader.h"
#include "UI/Text/Font.h"
#include <imgui.h>
#include <iostream>
#include <filesystem>
#include <cstring>
#include <fstream>

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

    std::string ProjectBrowserPanel::KeyFromPath(const std::filesystem::path &path) { return path.stem().string(); }

    std::vector<AssetEntry> ProjectBrowserPanel::ScanDirectory(const std::string &subDir, const std::string &extension) {
        std::vector<AssetEntry> entries;
        const auto resolved = IO::VFS::Resolve(subDir);
        if (resolved.empty() || !std::filesystem::exists(resolved))
            return entries;

        for (const auto &entry : std::filesystem::directory_iterator(resolved)) {
            if (entry.is_regular_file() && entry.path().extension() == extension) {
                auto relPath = std::filesystem::relative(entry.path(), IO::VFS::GetProjectRoot());
                std::string relStr = relPath.string();
                for (auto &c : relStr)
                    if (c == '\\')
                        c = '/';
                entries.push_back({entry.path().stem().string(), relStr});
            }
        }
        return entries;
    }

    void ProjectBrowserPanel::OnImGuiRender() {
        ImGui::Begin("Project Browser");
        m_IsHovered = ImGui::IsWindowHovered();

        // Search bar
        ImGui::InputText("##Search", m_SearchBuffer, sizeof(m_SearchBuffer));
        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            m_SearchBuffer[0] = '\0';
        }
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

        if (ImGui::CollapsingHeader("Textures")) {
            DrawTextureSection(resources);
        }
        if (ImGui::CollapsingHeader("Shaders")) {
            DrawShaderSection(resources);
        }
        if (ImGui::CollapsingHeader("Meshes")) {
            DrawMeshSection(resources);
        }
        if (ImGui::CollapsingHeader("Materials")) {
            DrawMaterialSection(resources);
        }
        if (ImGui::CollapsingHeader("Fonts")) {
            DrawFontSection(resources);
        }
        if (ImGui::CollapsingHeader("Scripts")) {
            DrawFileSection("Scripts", std::string(Core::SCRIPT_PATH), std::string(Core::SCRIPT_FILE_EXTENSION), ".obsl,txt", "Script Files");
        }
        if (ImGui::CollapsingHeader("Maps")) {
            DrawFileSection("Maps", std::string(Core::MAP_PATH), std::string(Core::MAP_FILE_EXTENSION), ".obmap", "Map Files");
        }
        if (ImGui::CollapsingHeader("Scenes")) {
            DrawFileSection("Scenes", std::string(Core::SCENE_PATH), ".json", ".json", "Scene Files");
        }

        DrawDeleteConfirmPopup(resources);

        ImGui::End();
    }


    template <typename T>
    void ProjectBrowserPanel::DrawResourceSection(Core::ResourceManager &resources, const std::unordered_map<std::string, std::shared_ptr<T>> &allItems, const AssetType assetType, const char *childId,
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
                    if (!ContainsSearch(id.c_str(), m_SearchBuffer))
                        continue;

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
                        if (ImGui::SmallButton("Cancel")) {
                            m_RenamingKey.clear();
                        }
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
                        if (!isEngineBuiltin) {
                            if (ImGui::SmallButton("Remove")) {
                                m_DeleteConfirmKey = id;
                                m_DeleteConfirmType = assetType;
                            }
                        }
                    }

                    ImGui::EndGroup();
                    ImGui::EndGroup();

                    auto *dl = ImGui::GetWindowDrawList();
                    const auto min = ImGui::GetItemRectMin();
                    const auto max = ImGui::GetItemRectMax();
                    dl->AddRect(ImVec2(min.x - 2, min.y - 2), ImVec2(max.x + 2, max.y + 2), ImGui::GetColorU32(ImGuiCol_Border));
                    ImGui::PopID();
                    itemCount++;
                }
            } else {
                for (const auto &[id, asset] : allItems) {
                    if (!ContainsSearch(id.c_str(), m_SearchBuffer))
                        continue;

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
                        if (ImGui::SmallButton("Cancel")) {
                            m_RenamingKey.clear();
                        }
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
            auto ptr = resources.Get<T>(oldKey);
            resources.Unload<T>(oldKey);
            resources.LoadFromFactory<T>(newKey, [ptr] { return ptr; });
            LOG_INFO(LOG_WHO, std::string("Renamed ") + typeName + " '" + oldKey + "' -> '" + newKey + "'");
        }

        if (allItems.empty() || (m_SearchBuffer[0] != '\0' && pendingRenames.empty())) {
            ImGui::TextDisabled("%s", emptyText);
        }
    }

    void ProjectBrowserPanel::DrawTextureSection(Core::ResourceManager &resources) {
        const auto &allTextures = resources.GetAll<Rendering::Texture>();
        if (m_SearchBuffer[0] != '\0') {
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear")) {
                m_SearchBuffer[0] = '\0';
            }
        }

        DrawResourceSection(
                resources, allTextures, AssetType::Texture, "##texList", 130.0f, "No textures imported.", "texture",
                [](const std::shared_ptr<Rendering::Texture> &tex) {
                    if (tex)
                        Core::Utils::UI::ImGuiImageFlipped(tex->GetID(), ImVec2(64, 64));
                    else
                        ImGui::Button("T", ImVec2(64, 64));
                },
                [this](const std::string &id, Core::ResourceManager &res) {
                    if (ImGui::SmallButton("Replace"))
                        ReplaceTexture(res, id);
                });

        ImGui::Spacing();
        if (ImGui::SmallButton("Import Texture")) {
            ImportTexture(resources);
        }
    }

    void ProjectBrowserPanel::DrawShaderSection(Core::ResourceManager &resources) {
        const auto &allShaders = resources.GetAll<Rendering::Shader>();

        DrawResourceSection(
                resources, allShaders, AssetType::Shader, "##shaderList", 130.0f, "No shaders imported.", "shader", [](const std::shared_ptr<Rendering::Shader> &) { ImGui::Button("S", ImVec2(64, 64)); },
                [this](const std::string &id, Core::ResourceManager &res) {
                    if (ImGui::SmallButton("Replace"))
                        ReplaceShader(res, id);
                });

        ImGui::Spacing();
        if (ImGui::SmallButton("Import Shader")) {
            ImportShader(resources);
        }
    }

    void ProjectBrowserPanel::DrawMeshSection(Core::ResourceManager &resources) {
        const auto &allMeshes = resources.GetAll<Rendering::Mesh>();

        DrawResourceSection(
                resources, allMeshes, AssetType::Mesh, "##meshList", 100.0f, "No meshes registered.", "mesh", [](const std::shared_ptr<Rendering::Mesh> &) { ImGui::Button("M", ImVec2(64, 64)); },
                [](const std::string &, Core::ResourceManager &) {
                });

        ImGui::Spacing();
        ImGui::SeparatorText("Create Mesh");

        constexpr const char *meshTypes[] = {"Quad", "PointTopHex", "ETriang", "Ellipse", "Circle", "Pentagon", "Hexagon", "Octagon", "Ring", "Sector", "Diamond"};
        ImGui::Combo("Type", &m_SelectedMeshFactory, meshTypes, IM_ARRAYSIZE(meshTypes));
        ImGui::InputText("ID##mesh", m_MeshNameBuffer, sizeof(m_MeshNameBuffer));

        if (ImGui::Button("Create Mesh")) {
            if (m_MeshNameBuffer[0] == '\0') {
                snprintf(m_MeshNameBuffer, sizeof(m_MeshNameBuffer), "%s_mesh", meshTypes[m_SelectedMeshFactory]);
            }
            if (const std::string id(m_MeshNameBuffer); resources.Get<Rendering::Mesh>(id)) {
                LOG_ERROR(LOG_WHO, "Mesh '" + id + "' already exists");
            } else {
                CreateMesh(resources);
            }
        }
    }

    void ProjectBrowserPanel::DrawMaterialSection(Core::ResourceManager &resources) {
        const auto &allMaterials = resources.GetAll<Rendering::Material>();

        DrawResourceSection(
                resources, allMaterials, AssetType::Material, "##matList", 100.0f, "No materials registered.", "material",
                [](const std::shared_ptr<Rendering::Material> &mat) {
                    if (mat && mat->texture)
                        Core::Utils::UI::ImGuiImageFlipped(mat->texture->GetID(), ImVec2(64, 64));
                    else
                        ImGui::Button("M", ImVec2(64, 64));
                },
                [](const std::string &, Core::ResourceManager &) {
                },
                [](const std::string &, const std::shared_ptr<Rendering::Material> &mat, Core::ResourceManager &res) {
                    const std::string tooltip = "Shader: " + (mat->shader ? res.GetKey(mat->shader) : "none") + "\nTexture: " + (mat->texture ? res.GetKey(mat->texture) : "none");
                    ImGui::SetTooltip("%s", tooltip.c_str());
                });

        ImGui::Spacing();
        ImGui::SeparatorText("Create Material");

        // Shader
        const auto &allShaders = resources.GetAll<Rendering::Shader>();
        std::vector<std::string> shaderKeys{"None"};
        for (const auto &[key, _] : allShaders) {
            (void)_;
            shaderKeys.push_back(key);
        }
        if (m_SelectedMaterialShaderIdx >= static_cast<int>(shaderKeys.size()))
            m_SelectedMaterialShaderIdx = 0;
        ImGui::Combo(
                "Shader##createMat", &m_SelectedMaterialShaderIdx,
                [](void *data, const int idx) -> const char * {
                    const auto &keys = *static_cast<std::vector<std::string> *>(data);
                    if (idx < 0 || idx >= static_cast<int>(keys.size()))
                        return "";
                    return keys[idx].c_str();
                },
                &shaderKeys, static_cast<int>(shaderKeys.size()));

        // Texture
        const auto &allTextures = resources.GetAll<Rendering::Texture>();
        std::vector<std::string> texKeys{"None"};
        for (const auto &[key, _] : allTextures) {
            (void)_;
            texKeys.push_back(key);
        }
        if (m_SelectedMaterialTextureIdx >= static_cast<int>(texKeys.size()))
            m_SelectedMaterialTextureIdx = 0;
        ImGui::Combo(
                "Texture##createMat", &m_SelectedMaterialTextureIdx,
                [](void *data, const int idx) -> const char * {
                    const auto &keys = *static_cast<std::vector<std::string> *>(data);
                    if (idx < 0 || idx >= static_cast<int>(keys.size()))
                        return "";
                    return keys[idx].c_str();
                },
                &texKeys, static_cast<int>(texKeys.size()));

        ImGui::ColorEdit4("Color##createMat", &m_MaterialColor.x, ImGuiColorEditFlags_NoInputs);
        ImGui::InputText("ID##mat", m_MaterialNameBuffer, sizeof(m_MaterialNameBuffer));

        if (ImGui::Button("Create Material")) {
            if (m_MaterialNameBuffer[0] == '\0') {
                LOG_ERROR(LOG_WHO, "Material name cannot be empty");
            } else {
                const std::string id(m_MaterialNameBuffer);
                if (resources.Get<Rendering::Material>(id)) {
                    LOG_ERROR(LOG_WHO, "Material '" + id + "' already exists");
                } else {
                    std::shared_ptr<Rendering::Shader> shader;
                    if (m_SelectedMaterialShaderIdx > 0 && m_SelectedMaterialShaderIdx < static_cast<int>(shaderKeys.size())) {
                        shader = resources.Get<Rendering::Shader>(shaderKeys[m_SelectedMaterialShaderIdx]);
                    }

                    std::shared_ptr<Rendering::Texture> texture;
                    if (m_SelectedMaterialTextureIdx > 0 && m_SelectedMaterialTextureIdx < static_cast<int>(texKeys.size())) {
                        texture = resources.Get<Rendering::Texture>(texKeys[m_SelectedMaterialTextureIdx]);
                    }

                    const auto mat =
                            resources.LoadFromFactory<Rendering::Material>(id, [shader, texture, color = m_MaterialColor] { return std::make_shared<Rendering::Material>(Rendering::Material{shader, texture, color}); });
                }
                // (void)mat; // mat is inside lambda, no need to suppress unused warning
                LOG_INFO(LOG_WHO, "Created material '" + id + "'");

                m_MaterialNameBuffer[0] = '\0';
                m_MaterialColor = {1.0f, 1.0f, 1.0f, 1.0f};
                m_SelectedMaterialShaderIdx = 0;
                m_SelectedMaterialTextureIdx = 0;
            }
        }
    }

    void ProjectBrowserPanel::DrawFontSection(Core::ResourceManager &resources) {
        const auto &allFonts = resources.GetAll<::UI::Font>();

        DrawResourceSection(
                resources, allFonts, AssetType::Font, "##fontList", 100.0f, "No fonts imported.", "font",
                [](const std::shared_ptr<::UI::Font> &font) {
                    ImGui::Text("%s", font ? "F" : "?");
                    if (font) {
                        ImGui::SameLine();
                        ImGui::TextDisabled("%upx%s", font->GetFontSize(), font->IsSDF() ? " SDF" : "");
                    }
                },
                [this](const std::string &id, Core::ResourceManager &res) {
                    if (ImGui::SmallButton("Replace"))
                        ReplaceFont(res, id);
                },
                [](const std::string &key, const std::shared_ptr<::UI::Font> &font, Core::ResourceManager &) {
                    if (font) {
                        ImGui::SetTooltip("%s\nSize: %u\nSDF: %s\nGlyphs: loaded", key.c_str(), font->GetFontSize(), font->IsSDF() ? "yes" : "no");
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

        if (ImGui::SmallButton("Import Font")) {
            ImportFont(resources);
        }
    }

    void ProjectBrowserPanel::ImportFont(Core::ResourceManager &resources) const {
        if (!m_EngineContext)
            return;

        const auto picked = Platform::FileDialogs::OpenFile(*m_EngineContext, {.filterName = "Font", .filterExt = "ttf,otf,woff,woff2"});
        if (!picked.has_value())
            return;

        auto finalPath = IO::AssetLoader::ImportAsset(picked.value(), "fonts");
        if (!finalPath.has_value())
            return;

        const std::string key = KeyFromPath(std::filesystem::path(finalPath.value()));
        if (resources.Get<::UI::Font>(key)) {
            LOG_WARN(LOG_WHO, "Font '" + key + "' already exists. Skipping");
            return;
        }

        auto font = resources.Load<::UI::Font>(key, finalPath.value(), static_cast<unsigned int>(m_FontSize), m_FontUseSDF, static_cast<unsigned int>(m_FontSDFSpread));
        std::thread([font] {
            font->LoadCPU();
            Rendering::Renderer::SubmitInitTask(::Platform::Threading::SmallTask([font] { font->InitGL(); }));
        }).detach();
        LOG_INFO(LOG_WHO, "Imported font '" + key + "' from " + finalPath.value());
    }

    void ProjectBrowserPanel::ReplaceFont(Core::ResourceManager &resources, const std::string &key) const {
        if (!m_EngineContext)
            return;

        const auto picked = Platform::FileDialogs::OpenFile(*m_EngineContext, {.filterName = "Font", .filterExt = "ttf,otf,woff,woff2"});
        if (!picked.has_value())
            return;

        auto finalPath = IO::AssetLoader::ImportAsset(picked.value(), "fonts");
        if (!finalPath.has_value())
            return;

        resources.Unload<::UI::Font>(key);
        auto font = resources.Load<::UI::Font>(key, finalPath.value(), static_cast<unsigned int>(m_FontSize), m_FontUseSDF, static_cast<unsigned int>(m_FontSDFSpread));
        std::thread([font] {
            font->LoadCPU();
            Rendering::Renderer::SubmitInitTask(::Platform::Threading::SmallTask([font] { font->InitGL(); }));
        }).detach();
        LOG_INFO(LOG_WHO, "Replaced font '" + key + "'");
    }

} // namespace Editor::UI

void Editor::UI::ProjectBrowserPanel::DrawFileSection(const char *label, const std::string &directory, const std::string &extension, const char *importFilter, const char *importFilterName) {
    bool isScripts = std::strcmp(label, "Scripts") == 0;

    char *newScriptBuf = m_NewScriptBuffer;

    auto entries = ScanDirectory(directory, extension);

    ImGui::PushID(label);
    if (ImGui::BeginChild("list", ImVec2(0, 120), true)) {
        if (m_ViewMode == ViewMode::Grid) {
            int itemsPerRow = static_cast<int>(ImGui::GetContentRegionAvail().x / 180.0f);
            if (itemsPerRow < 1)
                itemsPerRow = 1;
            int itemCount = 0;
            for (const auto &[name, virtualPath] : entries) {
                if (!ContainsSearch(virtualPath.c_str(), m_SearchBuffer))
                    continue;

                if (itemCount % itemsPerRow != 0)
                    ImGui::SameLine();

                ImGui::PushID(virtualPath.c_str());
                ImGui::BeginGroup();

                ImGui::Button("F", ImVec2(64, 64));

                ImGui::SameLine();

                ImGui::BeginGroup();
                ImGui::Text("%s", virtualPath.c_str());

                if (ImGui::SmallButton("Remove")) {
                    m_DeleteConfirmKey = virtualPath;
                    m_DeleteConfirmFilePath = virtualPath;
                    m_DeleteConfirmType = AssetType::Material; // dummy
                }

                ImGui::EndGroup();
                ImGui::EndGroup();
                auto *dl = ImGui::GetWindowDrawList();
                auto min = ImGui::GetItemRectMin();
                auto max = ImGui::GetItemRectMax();
                dl->AddRect(ImVec2(min.x - 2, min.y - 2), ImVec2(max.x + 2, max.y + 2), ImGui::GetColorU32(ImGuiCol_Border));
                ImGui::PopID(); // entry
                itemCount++;
            }
        } else {
            for (const auto &[name, virtualPath] : entries) {
                if (!ContainsSearch(virtualPath.c_str(), m_SearchBuffer))
                    continue;

                ImGui::PushID(virtualPath.c_str());
                ImGui::Text("%s", virtualPath.c_str());
                ImGui::SameLine();

                if (ImGui::SmallButton("Remove")) {
                    m_DeleteConfirmKey = virtualPath;
                    m_DeleteConfirmFilePath = virtualPath;
                    m_DeleteConfirmType = AssetType::Material; // dummy
                }
                ImGui::PopID(); // entry
            }
        }
    }
    ImGui::EndChild();
    ImGui::PopID(); // label

    if (entries.empty() || (m_SearchBuffer[0] != '\0' && std::ranges::all_of(entries, [this](const AssetEntry &e) { return !ContainsSearch(e.virtualPath.c_str(), m_SearchBuffer); }))) {
        ImGui::TextDisabled("No %s found.", importFilterName);
    }

    ImGui::Spacing();
    if (ImGui::SmallButton(("Import " + std::string(importFilterName)).c_str())) {
        ImportFile(directory, importFilter, importFilterName);
    }

    if (isScripts) {
        ImGui::SameLine();
        ImGui::InputText("##newScript", newScriptBuf, sizeof(m_NewScriptBuffer));
        ImGui::SameLine();
        if (ImGui::SmallButton("New Script")) {
            if (newScriptBuf[0] != '\0') {
                std::string filename(newScriptBuf);
                if (!filename.ends_with(".obsl"))
                    filename += ".obsl";
                if (auto dir = IO::VFS::Resolve(std::string(Core::SCRIPT_PATH)); !dir.empty()) {
                    std::filesystem::create_directories(dir);
                    if (auto filePath = dir / filename; !std::filesystem::exists(filePath)) {
                        std::ofstream ofs(filePath);
                        ofs << "// " << newScriptBuf << "\n";
                        ofs.close();
                        LOG_INFO(LOG_WHO, "Created script '" + filename + "'");
                    } else {
                        LOG_ERROR(LOG_WHO, "Script '" + filename + "' already exists");
                    }
                }
                newScriptBuf[0] = '\0';
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
            ImGui::TextDisabled("This will permanently delete the file from disk.");
        }
        ImGui::Separator();

        if (ImGui::Button("Yes, Remove", ImVec2(120, 0))) {
            // File-based asset
            if (!m_DeleteConfirmFilePath.empty()) {
                if (const auto absPath = IO::VFS::Resolve(m_DeleteConfirmFilePath); !absPath.empty() && std::filesystem::remove(absPath)) {
                    LOG_INFO(LOG_WHO, "Deleted file '" + m_DeleteConfirmFilePath + "'");
                } else {
                    LOG_ERROR(LOG_WHO, "Failed to delete '" + m_DeleteConfirmFilePath + "'");
                }
            }
            // RM (as in resource manager, not rm) asset
            else {
                switch (m_DeleteConfirmType) {
                    case AssetType::Texture:
                        resources.Unload<Rendering::Texture>(m_DeleteConfirmKey);
                        LOG_INFO(LOG_WHO, "Removed texture '" + m_DeleteConfirmKey + "'");
                        break;
                    case AssetType::Shader:
                        resources.Unload<Rendering::Shader>(m_DeleteConfirmKey);
                        LOG_INFO(LOG_WHO, "Removed shader '" + m_DeleteConfirmKey + "'");
                        break;
                    case AssetType::Mesh:
                        resources.Unload<Rendering::Mesh>(m_DeleteConfirmKey);
                        LOG_INFO(LOG_WHO, "Removed mesh '" + m_DeleteConfirmKey + "'");
                        break;
                    case AssetType::Material:
                        resources.Unload<Rendering::Material>(m_DeleteConfirmKey);
                        LOG_INFO(LOG_WHO, "Removed material '" + m_DeleteConfirmKey + "'");
                        break;
                    case AssetType::Font:
                        resources.Unload<::UI::Font>(m_DeleteConfirmKey);
                        LOG_INFO(LOG_WHO, "Removed font '" + m_DeleteConfirmKey + "'");
                        break;
                }
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
    if (!picked.has_value())
        return;

    auto finalPath = IO::AssetLoader::ImportAsset(picked.value(), "textures");
    if (!finalPath.has_value())
        return;

    const std::string key = KeyFromPath(std::filesystem::path(finalPath.value()));
    if (resources.Get<Rendering::Texture>(key)) {
        LOG_WARN(LOG_WHO, "Texture '" + key + "' already exists. Skipping");
        return;
    }

    auto tex = resources.Load<Rendering::Texture>(key, finalPath.value());
    Rendering::Renderer::SubmitInitTask(::Platform::Threading::SmallTask([tex] { tex->InitGL(); }));
    LOG_INFO(LOG_WHO, "Imported texture '" + key + "' from " + finalPath.value());
}

void Editor::UI::ProjectBrowserPanel::ImportShader(Core::ResourceManager &resources) const {
    if (!m_EngineContext)
        return;

    const auto vertPicked = Platform::FileDialogs::OpenFile(*m_EngineContext, {.filterName = "Vertex Shader", .filterExt = "vert,glsl"});
    if (!vertPicked.has_value())
        return;

    const auto fragPicked = Platform::FileDialogs::OpenFile(*m_EngineContext, {.filterName = "Fragment Shader", .filterExt = "frag,glsl"});
    if (!fragPicked.has_value())
        return;

    auto finalVert = IO::AssetLoader::ImportAsset(vertPicked.value(), "shaders");
    auto finalFrag = IO::AssetLoader::ImportAsset(fragPicked.value(), "shaders");
    if (!finalVert.has_value() || !finalFrag.has_value())
        return;

    const std::string key = KeyFromPath(std::filesystem::path(finalVert.value()));
    if (resources.Get<Rendering::Shader>(key)) {
        LOG_WARN(LOG_WHO, "Shader '" + key + "' already exists. Skipping");
        return;
    }

    auto shader = resources.Load<Rendering::Shader>(key, finalVert.value(), finalFrag.value());
    Rendering::Renderer::SubmitInitTask(::Platform::Threading::SmallTask([shader] { shader->InitGL(); }));
    LOG_INFO(LOG_WHO, "Imported shader '" + key + "'");
}

void Editor::UI::ProjectBrowserPanel::ReplaceTexture(Core::ResourceManager &resources, const std::string &key) const {
    if (!m_EngineContext)
        return;

    const auto picked = Platform::FileDialogs::OpenFile(*m_EngineContext, {.filterName = "Image", .filterExt = "png,jpg,jpeg,bmp,tga"});
    if (!picked.has_value())
        return;

    auto finalPath = IO::AssetLoader::ImportAsset(picked.value(), "textures");
    if (!finalPath.has_value())
        return;

    resources.Unload<Rendering::Texture>(key);
    auto tex = resources.Load<Rendering::Texture>(key, finalPath.value());
    Rendering::Renderer::SubmitInitTask(::Platform::Threading::SmallTask([tex] { tex->InitGL(); }));
    LOG_INFO(LOG_WHO, "Replaced texture '" + key + "'");
}

void Editor::UI::ProjectBrowserPanel::ReplaceShader(Core::ResourceManager &resources, const std::string &key) const {
    if (!m_EngineContext)
        return;

    const auto vertPicked = Platform::FileDialogs::OpenFile(*m_EngineContext, {.filterName = "Vertex Shader", .filterExt = "vert,glsl"});
    if (!vertPicked.has_value())
        return;

    const auto fragPicked = Platform::FileDialogs::OpenFile(*m_EngineContext, {.filterName = "Fragment Shader", .filterExt = "frag,glsl"});
    if (!fragPicked.has_value())
        return;

    auto finalVert = IO::AssetLoader::ImportAsset(vertPicked.value(), "shaders");
    auto finalFrag = IO::AssetLoader::ImportAsset(fragPicked.value(), "shaders");
    if (!finalVert.has_value() || !finalFrag.has_value())
        return;

    resources.Unload<Rendering::Shader>(key);
    auto shader = resources.Load<Rendering::Shader>(key, finalVert.value(), finalFrag.value());
    Rendering::Renderer::SubmitInitTask(::Platform::Threading::SmallTask([shader] { shader->InitGL(); }));
    LOG_INFO(LOG_WHO, "Replaced shader '" + key + "'");
}

void Editor::UI::ProjectBrowserPanel::CreateMesh(Core::ResourceManager &resources) {
    const std::string id(m_MeshNameBuffer);
    if (id.empty())
        return;

    Rendering::MeshData data;
    constexpr const char *meshTypes[] = {"Quad", "PointTopHex", "ETriang", "Ellipse", "Circle", "Pentagon", "Hexagon", "Octagon", "Ring", "Sector", "Diamond"};
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
    }

    auto mesh = resources.LoadFromFactory<Rendering::Mesh>(id, [data = std::move(data), meshTypes, factoryIdx = m_SelectedMeshFactory] {
        auto m = std::make_shared<Rendering::Mesh>(data);
        m->SetFactoryId(meshTypes[factoryIdx]);
        return m;
    });

    Rendering::Renderer::SubmitInitTask(::Platform::Threading::SmallTask([mesh] { mesh->InitGL(); }));
    m_MeshNameBuffer[0] = '\0';
    LOG_INFO(LOG_WHO, "Created mesh '" + id + "'");
}

void Editor::UI::ProjectBrowserPanel::ImportFile(const std::string &targetSubDir, const char *filterExt, const char *filterName) const {
    if (!m_EngineContext)
        return;

    const auto picked = Platform::FileDialogs::OpenFile(*m_EngineContext, {.filterName = filterName, .filterExt = filterExt});
    if (!picked.has_value())
        return;

    IO::AssetLoader::ImportAsset(picked.value(), targetSubDir);
}
#pragma pop_macro("LOG_WHO")
