#include "ProjectBrowserPanel.h"

#include "Core/Constants.h"
#include "Editor/FileDialogs.h"
#include "IO/VFS.h"
#include "IO/AssetLoader.h"
#include "Rendering/Material.h"
#include "Rendering/Mesh.h"
#include "Rendering/MeshFactory.h"
#include "Rendering/Renderer.h"
#include "Rendering/Texture.h"
#include "Rendering/Shader.h"
#include <imgui.h>
#include <iostream>
#include <filesystem>
#include <cstring>
#include <fstream>

namespace Editor::UI {

    static bool ContainsSearch(const char *haystack, const char *needle) {
        if (needle[0] == '\0')
            return true; // empty search = show all
        const std::string hay(haystack ? haystack : "");
        const std::string need(needle);
        return hay.find(need) != std::string::npos;
    }

    std::string ProjectBrowserPanel::KeyFromPath(const std::filesystem::path &path) { return path.stem().string(); }

    std::vector<AssetEntry> ProjectBrowserPanel::ScanDirectory(const std::string &subDir,
                                                               const std::string &extension) const {
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
        if (ImGui::CollapsingHeader("Scripts")) {
            DrawFileSection("Scripts", std::string(Core::SCRIPT_PATH), std::string(Core::SCRIPT_FILE_EXTENSION),
                            ".obsl,txt", "Script Files");
        }
        if (ImGui::CollapsingHeader("Maps")) {
            DrawFileSection("Maps", std::string(Core::MAP_PATH), std::string(Core::MAP_FILE_EXTENSION), ".obmap",
                            "Map Files");
        }
        if (ImGui::CollapsingHeader("Scenes")) {
            DrawFileSection("Scenes", std::string(Core::SCENE_PATH), ".json", ".json", "Scene Files");
        }

        DrawDeleteConfirmPopup(resources);

        ImGui::End();
    }

    // textures are loaded with stbi flip on so they must be unflipped
    static void ImGuiImageFlipped(const GLuint textureID, const ImVec2 &size) {
        ImDrawList *drawList = ImGui::GetWindowDrawList();
        const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        drawList->AddImage(textureID, cursorPos, ImVec2(cursorPos.x + size.x, cursorPos.y + size.y), ImVec2(0, 1),
                           ImVec2(1, 0));
        ImGui::Dummy(size);
    }


    void ProjectBrowserPanel::DrawTextureSection(Core::ResourceManager &resources) {
        const auto &allTextures = resources.GetAll<Rendering::Texture>();
        struct RenameOp {
            std::string oldKey;
            std::string newKey;
        };
        std::vector<RenameOp> pendingRenames;

        // filter by search query
        bool showSearchClear = false;
        if (m_SearchBuffer[0] != '\0') {
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear")) {
                m_SearchBuffer[0] = '\0';
            }
            showSearchClear = true;
        }

        if (ImGui::BeginChild("##texList", ImVec2(0, 130), true)) {
            if (m_ViewMode == ViewMode::Grid) {
                // grid
                int itemsPerRow = static_cast<int>(ImGui::GetContentRegionAvail().x / 100.0f);
                if (itemsPerRow < 1)
                    itemsPerRow = 1;
                int itemCount = 0;
                for (const auto &[id, tex] : allTextures) {
                    if (!ContainsSearch(id.c_str(), m_SearchBuffer))
                        continue;

                    if (itemCount % itemsPerRow != 0)
                        ImGui::SameLine();

                    ImGui::PushID(id.c_str());
                    ImGui::BeginGroup();

                    // thumbnail
                    if (tex) {
                        ImGuiImageFlipped(tex->GetID(), ImVec2(64, 64));
                    } else {
                        ImGui::Button("T", ImVec2(64, 64));
                    }

                    // Label and actions
                    ImGui::Text("%s", id.c_str());

                    // action buttons
                    if (m_RenamingKey == id) {
                        if (m_RenameJustActivated) {
                            ImGui::SetKeyboardFocusHere();
                            m_RenameJustActivated = false;
                        }
                        ImGui::SetNextItemWidth(80.0f);
                        ImGui::InputText("##rename", m_RenameBuffer, sizeof(m_RenameBuffer));
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Save")) {
                            if (m_RenameBuffer[0] != '\0' && !resources.Get<Rendering::Texture>(m_RenameBuffer)) {
                                pendingRenames.push_back({id, m_RenameBuffer});
                            }
                            m_RenamingKey.clear();
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Cancel")) {
                            m_RenamingKey.clear();
                        }
                    } else {
                        if (ImGui::SmallButton("Rename")) {
                            m_RenamingKey = id;
                            m_RenameJustActivated = true;
                            strncpy(m_RenameBuffer, id.c_str(), sizeof(m_RenameBuffer));
                            m_RenameBuffer[sizeof(m_RenameBuffer) - 1] = '\0';
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Replace")) {
                            ReplaceTexture(resources, id);
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Remove")) {
                            m_DeleteConfirmKey = id;
                            m_DeleteConfirmType = AssetType::Texture;
                        }
                    }

                    ImGui::EndGroup();
                    ImGui::PopID();
                    itemCount++;
                }
            } else {
                // list
                for (const auto &id : allTextures | std::views::keys) {
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
                            if (m_RenameBuffer[0] != '\0' && !resources.Get<Rendering::Texture>(m_RenameBuffer)) {
                                pendingRenames.push_back({id, m_RenameBuffer});
                            }
                            m_RenamingKey.clear();
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Cancel")) {
                            m_RenamingKey.clear();
                        }
                    } else {
                        ImGui::Text("%s", id.c_str());
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Rename")) {
                            m_RenamingKey = id;
                            m_RenameJustActivated = true;
                            strncpy(m_RenameBuffer, id.c_str(), sizeof(m_RenameBuffer));
                            m_RenameBuffer[sizeof(m_RenameBuffer) - 1] = '\0';
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Replace")) {
                            ReplaceTexture(resources, id);
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Remove")) {
                            m_DeleteConfirmKey = id;
                            m_DeleteConfirmType = AssetType::Texture;
                        }
                    }

                    ImGui::PopID();
                }
            }
        }
        ImGui::EndChild();

        // apply pending renames
        for (const auto &[oldKey, newKey] : pendingRenames) {
            auto ptr = resources.Get<Rendering::Texture>(oldKey);
            resources.Unload<Rendering::Texture>(oldKey);
            resources.LoadFromFactory<Rendering::Texture>(newKey, [ptr] { return ptr; });
            std::cout << "[ProjectBrowser] Renamed texture '" << oldKey << "' -> '" << newKey << "'\n";
        }

        if (allTextures.empty() || (m_SearchBuffer[0] != '\0' && pendingRenames.empty())) {
            ImGui::TextDisabled("No textures imported.");
        }

        ImGui::Spacing();
        if (ImGui::SmallButton("Import Texture")) {
            ImportTexture(resources);
        }
    }

    void ProjectBrowserPanel::DrawShaderSection(Core::ResourceManager &resources) {
        const auto &allShaders = resources.GetAll<Rendering::Shader>();
        struct RenameOp {
            std::string oldKey;
            std::string newKey;
        };
        std::vector<RenameOp> pendingRenames;

        if (ImGui::BeginChild("##shaderList", ImVec2(0, 130), true)) {
            if (m_ViewMode == ViewMode::Grid) {
                int itemsPerRow = static_cast<int>(ImGui::GetContentRegionAvail().x / 100.0f);
                if (itemsPerRow < 1)
                    itemsPerRow = 1;
                int itemCount = 0;
                for (const auto &id : allShaders | std::views::keys) {
                    if (!ContainsSearch(id.c_str(), m_SearchBuffer))
                        continue;

                    if (itemCount % itemsPerRow != 0)
                        ImGui::SameLine();

                    ImGui::PushID(id.c_str());
                    ImGui::BeginGroup();

                    ImGui::Button("S", ImVec2(64, 64));

                    ImGui::Text("%s", id.c_str());

                    // actions
                    if (m_RenamingKey == id) {
                        if (m_RenameJustActivated) {
                            ImGui::SetKeyboardFocusHere();
                            m_RenameJustActivated = false;
                        }
                        ImGui::SetNextItemWidth(80.0f);
                        ImGui::InputText("##rename", m_RenameBuffer, sizeof(m_RenameBuffer));
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Save")) {
                            if (m_RenameBuffer[0] != '\0' && !resources.Get<Rendering::Shader>(m_RenameBuffer)) {
                                pendingRenames.push_back({id, m_RenameBuffer});
                            }
                            m_RenamingKey.clear();
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Cancel")) {
                            m_RenamingKey.clear();
                        }
                    } else {
                        if (ImGui::SmallButton("Rename")) {
                            m_RenamingKey = id;
                            m_RenameJustActivated = true;
                            strncpy(m_RenameBuffer, id.c_str(), sizeof(m_RenameBuffer));
                            m_RenameBuffer[sizeof(m_RenameBuffer) - 1] = '\0';
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Replace")) {
                            ReplaceShader(resources, id);
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Remove")) {
                            m_DeleteConfirmKey = id;
                            m_DeleteConfirmType = AssetType::Shader;
                        }
                    }

                    ImGui::EndGroup();
                    ImGui::PopID();
                    itemCount++;
                }
            } else {
                for (const auto &id : allShaders | std::views::keys) {
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
                            if (m_RenameBuffer[0] != '\0' && !resources.Get<Rendering::Shader>(m_RenameBuffer)) {
                                pendingRenames.push_back({id, m_RenameBuffer});
                            }
                            m_RenamingKey.clear();
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Cancel")) {
                            m_RenamingKey.clear();
                        }
                    } else {
                        ImGui::Text("%s", id.c_str());
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Rename")) {
                            m_RenamingKey = id;
                            m_RenameJustActivated = true;
                            strncpy(m_RenameBuffer, id.c_str(), sizeof(m_RenameBuffer));
                            m_RenameBuffer[sizeof(m_RenameBuffer) - 1] = '\0';
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Replace")) {
                            ReplaceShader(resources, id);
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Remove")) {
                            m_DeleteConfirmKey = id;
                            m_DeleteConfirmType = AssetType::Shader;
                        }
                    }

                    ImGui::PopID();
                }
            }
        }
        ImGui::EndChild();

        for (const auto &[oldKey, newKey] : pendingRenames) {
            auto ptr = resources.Get<Rendering::Shader>(oldKey);
            resources.Unload<Rendering::Shader>(oldKey);
            resources.LoadFromFactory<Rendering::Shader>(newKey, [ptr] { return ptr; });
            std::cout << "[ProjectBrowser] Renamed shader '" << oldKey << "' -> '" << newKey << "'\n";
        }

        if (allShaders.empty() || (m_SearchBuffer[0] != '\0' && pendingRenames.empty())) {
            ImGui::TextDisabled("No shaders imported.");
        }

        ImGui::Spacing();
        if (ImGui::SmallButton("Import Shader")) {
            ImportShader(resources);
        }
    }

    void ProjectBrowserPanel::DrawMeshSection(Core::ResourceManager &resources) {
        const auto &allMeshes = resources.GetAll<Rendering::Mesh>();
        struct RenameOp {
            std::string oldKey;
            std::string newKey;
        };
        std::vector<RenameOp> pendingRenames;

        if (ImGui::BeginChild("##meshList", ImVec2(0, 100), true)) {
            if (m_ViewMode == ViewMode::Grid) {
                int itemsPerRow = static_cast<int>(ImGui::GetContentRegionAvail().x / 100.0f);
                if (itemsPerRow < 1)
                    itemsPerRow = 1;
                int itemCount = 0;
                for (const auto &id : allMeshes | std::views::keys) {
                    if (!ContainsSearch(id.c_str(), m_SearchBuffer))
                        continue;

                    if (itemCount % itemsPerRow != 0)
                        ImGui::SameLine();

                    ImGui::PushID(id.c_str());
                    ImGui::BeginGroup();

                    ImGui::Button("M", ImVec2(64, 64));

                    ImGui::Text("%s", id.c_str());

                    // actions
                    if (m_RenamingKey == id) {
                        if (m_RenameJustActivated) {
                            ImGui::SetKeyboardFocusHere();
                            m_RenameJustActivated = false;
                        }
                        ImGui::SetNextItemWidth(80.0f);
                        ImGui::InputText("##rename", m_RenameBuffer, sizeof(m_RenameBuffer));
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Save")) {
                            if (m_RenameBuffer[0] != '\0' && !resources.Get<Rendering::Mesh>(m_RenameBuffer)) {
                                pendingRenames.push_back({id, m_RenameBuffer});
                            }
                            m_RenamingKey.clear();
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Cancel")) {
                            m_RenamingKey.clear();
                        }
                    } else {
                        if (ImGui::SmallButton("Rename")) {
                            m_RenamingKey = id;
                            m_RenameJustActivated = true;
                            strncpy(m_RenameBuffer, id.c_str(), sizeof(m_RenameBuffer));
                            m_RenameBuffer[sizeof(m_RenameBuffer) - 1] = '\0';
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Remove")) {
                            m_DeleteConfirmKey = id;
                            m_DeleteConfirmType = AssetType::Mesh;
                        }
                    }

                    ImGui::EndGroup();
                    ImGui::PopID();
                    itemCount++;
                }
            } else {
                for (const auto &id : allMeshes | std::views::keys) {
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
                            if (m_RenameBuffer[0] != '\0' && !resources.Get<Rendering::Mesh>(m_RenameBuffer)) {
                                pendingRenames.push_back({id, m_RenameBuffer});
                            }
                            m_RenamingKey.clear();
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Cancel")) {
                            m_RenamingKey.clear();
                        }
                    } else {
                        ImGui::Text("%s", id.c_str());
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Rename")) {
                            m_RenamingKey = id;
                            m_RenameJustActivated = true;
                            strncpy(m_RenameBuffer, id.c_str(), sizeof(m_RenameBuffer));
                            m_RenameBuffer[sizeof(m_RenameBuffer) - 1] = '\0';
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Remove")) {
                            m_DeleteConfirmKey = id;
                            m_DeleteConfirmType = AssetType::Mesh;
                        }
                    }

                    ImGui::PopID();
                }
            }
        }
        ImGui::EndChild();

        for (const auto &[oldKey, newKey] : pendingRenames) {
            auto ptr = resources.Get<Rendering::Mesh>(oldKey);
            resources.Unload<Rendering::Mesh>(oldKey);
            resources.LoadFromFactory<Rendering::Mesh>(newKey, [ptr] { return ptr; });
            std::cout << "[ProjectBrowser] Renamed mesh '" << oldKey << "' -> '" << newKey << "'\n";
        }

        if (allMeshes.empty() || (m_SearchBuffer[0] != '\0' && pendingRenames.empty())) {
            ImGui::TextDisabled("No meshes registered.");
        }

        ImGui::Spacing();
        ImGui::SeparatorText("Create Mesh");

        constexpr const char *meshTypes[] = {"Quad",    "PointTopHex", "ETriang", "Ellipse", "Circle", "Pentagon",
                                             "Hexagon", "Octagon",     "Ring",    "Sector",  "Diamond"};
        ImGui::Combo("Type", &m_SelectedMeshFactory, meshTypes, IM_ARRAYSIZE(meshTypes));
        ImGui::InputText("ID##mesh", m_MeshNameBuffer, sizeof(m_MeshNameBuffer));

        if (ImGui::Button("Create Mesh")) {
            if (m_MeshNameBuffer[0] == '\0') {
                snprintf(m_MeshNameBuffer, sizeof(m_MeshNameBuffer), "%s_mesh", meshTypes[m_SelectedMeshFactory]);
            }
            const std::string id(m_MeshNameBuffer);
            if (resources.Get<Rendering::Mesh>(id)) {
                std::cerr << "[ProjectBrowser] Mesh '" << id << "' already exists.\n";
            } else {
                CreateMesh(resources);
            }
        }
    }

    void ProjectBrowserPanel::DrawMaterialSection(Core::ResourceManager &resources) {
        const auto &allMaterials = resources.GetAll<Rendering::Material>();
        struct RenameOp {
            std::string oldKey;
            std::string newKey;
        };
        std::vector<RenameOp> pendingRenames;

        if (ImGui::BeginChild("##matList", ImVec2(0, 100), true)) {
            if (m_ViewMode == ViewMode::Grid) {
                int itemsPerRow = static_cast<int>(ImGui::GetContentRegionAvail().x / 100.0f);
                if (itemsPerRow < 1)
                    itemsPerRow = 1;
                int itemCount = 0;
                for (const auto &[id, mat] : allMaterials) {
                    if (!ContainsSearch(id.c_str(), m_SearchBuffer))
                        continue;

                    if (itemCount % itemsPerRow != 0)
                        ImGui::SameLine();

                    ImGui::PushID(id.c_str());
                    ImGui::BeginGroup();

                    if (mat && mat->texture) {
                        ImGuiImageFlipped(mat->texture->GetID(), ImVec2(64, 64));
                    } else {
                        ImGui::Button("M", ImVec2(64, 64));
                    }

                    ImGui::Text("%s", id.c_str());


                    // actions
                    if (m_RenamingKey == id) {
                        if (m_RenameJustActivated) {
                            ImGui::SetKeyboardFocusHere();
                            m_RenameJustActivated = false;
                        }
                        ImGui::SetNextItemWidth(80.0f);
                        ImGui::InputText("##rename", m_RenameBuffer, sizeof(m_RenameBuffer));
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Save")) {
                            if (m_RenameBuffer[0] != '\0' && !resources.Get<Rendering::Material>(m_RenameBuffer)) {
                                pendingRenames.push_back({id, m_RenameBuffer});
                            }
                            m_RenamingKey.clear();
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Cancel")) {
                            m_RenamingKey.clear();
                        }
                    } else {
                        if (ImGui::SmallButton("Rename")) {
                            m_RenamingKey = id;
                            m_RenameJustActivated = true;
                            strncpy(m_RenameBuffer, id.c_str(), sizeof(m_RenameBuffer));
                            m_RenameBuffer[sizeof(m_RenameBuffer) - 1] = '\0';
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Remove")) {
                            m_DeleteConfirmKey = id;
                            m_DeleteConfirmType = AssetType::Material;
                        }
                    }

                    ImGui::EndGroup();
                    ImGui::PopID();
                    itemCount++;
                }
            } else {
                for (const auto &[id, mat] : allMaterials) {
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
                            if (m_RenameBuffer[0] != '\0' && !resources.Get<Rendering::Material>(m_RenameBuffer)) {
                                pendingRenames.push_back({id, m_RenameBuffer});
                            }
                            m_RenamingKey.clear();
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Cancel")) {
                            m_RenamingKey.clear();
                        }
                    } else {
                        ImGui::Text("%s", id.c_str());
                        if (ImGui::IsItemHovered()) {
                            std::string tooltip =
                                    "Shader: " + (mat->shader ? resources.GetKey(mat->shader) : "none") +
                                    "\nTexture: " + (mat->texture ? resources.GetKey(mat->texture) : "none");
                            ImGui::SetTooltip("%s", tooltip.c_str());
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Rename")) {
                            m_RenamingKey = id;
                            m_RenameJustActivated = true;
                            strncpy(m_RenameBuffer, id.c_str(), sizeof(m_RenameBuffer));
                            m_RenameBuffer[sizeof(m_RenameBuffer) - 1] = '\0';
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Remove")) {
                            m_DeleteConfirmKey = id;
                            m_DeleteConfirmType = AssetType::Material;
                        }
                    }

                    ImGui::PopID();
                }
            }
        }
        ImGui::EndChild();

        for (const auto &[oldKey, newKey] : pendingRenames) {
            auto ptr = resources.Get<Rendering::Material>(oldKey);
            resources.Unload<Rendering::Material>(oldKey);
            resources.LoadFromFactory<Rendering::Material>(newKey, [ptr] { return ptr; });
            std::cout << "[ProjectBrowser] Renamed material '" << oldKey << "' -> '" << newKey << "'\n";
        }

        if (allMaterials.empty() || (m_SearchBuffer[0] != '\0' && pendingRenames.empty())) {
            ImGui::TextDisabled("No materials registered.");
        }

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
                std::cerr << "[ProjectBrowser] Material name cannot be empty.\n";
            } else {
                const std::string id(m_MaterialNameBuffer);
                if (resources.Get<Rendering::Material>(id)) {
                    std::cerr << "[ProjectBrowser] Material '" << id << "' already exists.\n";
                } else {
                    std::shared_ptr<Rendering::Shader> shader;
                    if (m_SelectedMaterialShaderIdx > 0 &&
                        m_SelectedMaterialShaderIdx < static_cast<int>(shaderKeys.size())) {
                        shader = resources.Get<Rendering::Shader>(shaderKeys[m_SelectedMaterialShaderIdx]);
                    }

                    std::shared_ptr<Rendering::Texture> texture;
                    if (m_SelectedMaterialTextureIdx > 0 &&
                        m_SelectedMaterialTextureIdx < static_cast<int>(texKeys.size())) {
                        texture = resources.Get<Rendering::Texture>(texKeys[m_SelectedMaterialTextureIdx]);
                    }

                    const auto mat = resources.LoadFromFactory<Rendering::Material>(id, [shader, texture,
                                                                                         color = m_MaterialColor] {
                        return std::make_shared<Rendering::Material>(Rendering::Material{shader, texture, color});
                    });
                    (void)mat;
                    std::cout << "[ProjectBrowser] Created material '" << id << "'\n";

                    m_MaterialNameBuffer[0] = '\0';
                    m_MaterialColor = {1.0f, 1.0f, 1.0f, 1.0f};
                    m_SelectedMaterialShaderIdx = 0;
                    m_SelectedMaterialTextureIdx = 0;
                }
            }
        }
    }

    // file section

    void ProjectBrowserPanel::DrawFileSection(const char *label, const std::string &directory,
                                              const std::string &extension, const char *importFilter,
                                              const char *importFilterName) {
        bool isScripts = (std::strcmp(label, "Scripts") == 0);

        // "new script" buffer
        char *newScriptBuf = m_NewScriptBuffer;

        auto entries = ScanDirectory(directory, extension);

        ImGui::PushID(label);
        if (ImGui::BeginChild("list", ImVec2(0, 120), true)) {
            if (m_ViewMode == ViewMode::Grid) {
                int itemsPerRow = static_cast<int>(ImGui::GetContentRegionAvail().x / 100.0f);
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

                    ImGui::Text("%s", virtualPath.c_str());

                    if (ImGui::SmallButton("Remove")) {
                        m_DeleteConfirmKey = virtualPath;
                        m_DeleteConfirmFilePath = virtualPath;
                        m_DeleteConfirmType = AssetType::Material; // dummy
                    }

                    ImGui::EndGroup();
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

        if (entries.empty() || (m_SearchBuffer[0] != '\0' && std::ranges::all_of(entries, [this](const AssetEntry &e) {
                                    return !ContainsSearch(e.virtualPath.c_str(), m_SearchBuffer);
                                }))) {
            ImGui::TextDisabled("No %s found.", importFilterName);
        }

        ImGui::Spacing();
        if (ImGui::SmallButton(("Import " + std::string(importFilterName)).c_str())) {
            ImportFile(directory, importFilter, importFilterName);
        }

        // create a blank .obsl file
        if (isScripts) {
            ImGui::SameLine();
            ImGui::InputText("##newScript", newScriptBuf, sizeof(m_NewScriptBuffer));
            ImGui::SameLine();
            if (ImGui::SmallButton("New Script")) {
                if (newScriptBuf[0] != '\0') {
                    std::string filename(newScriptBuf);
                    if (!filename.ends_with(".obsl"))
                        filename += ".obsl";
                    auto dir = IO::VFS::Resolve(std::string(Core::SCRIPT_PATH));
                    if (!dir.empty()) {
                        std::filesystem::create_directories(dir);
                        auto filePath = dir / filename;
                        if (!std::filesystem::exists(filePath)) {
                            std::ofstream ofs(filePath);
                            ofs << "// " << newScriptBuf << "\n";
                            ofs.close();
                            std::cout << "[ProjectBrowser] Created script '" << filename << "'\n";
                        } else {
                            std::cerr << "[ProjectBrowser] Script '" << filename << "' already exists.\n";
                        }
                    }
                    newScriptBuf[0] = '\0';
                }
            }
        }
    }

    // delete confirmation
    void ProjectBrowserPanel::DrawDeleteConfirmPopup(Core::ResourceManager &resources) {
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
                    const auto absPath = IO::VFS::Resolve(m_DeleteConfirmFilePath);
                    if (!absPath.empty() && std::filesystem::remove(absPath)) {
                        std::cout << "[ProjectBrowser] Deleted file '" << m_DeleteConfirmFilePath << "'\n";
                    } else {
                        std::cerr << "[ProjectBrowser] Failed to delete '" << m_DeleteConfirmFilePath << "'\n";
                    }
                }
                // RM (as in resource manager, not rm) asset
                else {
                    switch (m_DeleteConfirmType) {
                        case AssetType::Texture:
                            resources.Unload<Rendering::Texture>(m_DeleteConfirmKey);
                            std::cout << "[ProjectBrowser] Removed texture '" << m_DeleteConfirmKey << "'\n";
                            break;
                        case AssetType::Shader:
                            resources.Unload<Rendering::Shader>(m_DeleteConfirmKey);
                            std::cout << "[ProjectBrowser] Removed shader '" << m_DeleteConfirmKey << "'\n";
                            break;
                        case AssetType::Mesh:
                            resources.Unload<Rendering::Mesh>(m_DeleteConfirmKey);
                            std::cout << "[ProjectBrowser] Removed mesh '" << m_DeleteConfirmKey << "'\n";
                            break;
                        case AssetType::Material:
                            resources.Unload<Rendering::Material>(m_DeleteConfirmKey);
                            std::cout << "[ProjectBrowser] Removed material '" << m_DeleteConfirmKey << "'\n";
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

    // import / create helpers
    void ProjectBrowserPanel::ImportTexture(Core::ResourceManager &resources) const {
        if (!m_EngineContext)
            return;

        const auto picked =
                FileDialogs::OpenFile(*m_EngineContext, {.filterName = "Image", .filterExt = "png,jpg,jpeg,bmp,tga"});
        if (!picked.has_value())
            return;

        auto finalPath = IO::AssetLoader::ImportAsset(picked.value(), "textures");
        if (!finalPath.has_value())
            return;

        const std::string key = KeyFromPath(std::filesystem::path(finalPath.value()));
        if (resources.Get<Rendering::Texture>(key)) {
            std::cerr << "[ProjectBrowser] Texture '" << key << "' already exists. Skipping.\n";
            return;
        }

        auto tex = resources.Load<Rendering::Texture>(key, finalPath.value());
        Rendering::Renderer::SubmitInitTask([tex] { tex->InitGL(); });
        std::cout << "[ProjectBrowser] Imported texture '" << key << "' from " << finalPath.value() << "\n";
    }

    void ProjectBrowserPanel::ImportShader(Core::ResourceManager &resources) const {
        if (!m_EngineContext)
            return;

        const auto vertPicked =
                FileDialogs::OpenFile(*m_EngineContext, {.filterName = "Vertex Shader", .filterExt = "vert,glsl"});
        if (!vertPicked.has_value())
            return;

        const auto fragPicked =
                FileDialogs::OpenFile(*m_EngineContext, {.filterName = "Fragment Shader", .filterExt = "frag,glsl"});
        if (!fragPicked.has_value())
            return;

        auto finalVert = IO::AssetLoader::ImportAsset(vertPicked.value(), "shaders");
        auto finalFrag = IO::AssetLoader::ImportAsset(fragPicked.value(), "shaders");
        if (!finalVert.has_value() || !finalFrag.has_value())
            return;

        const std::string key = KeyFromPath(std::filesystem::path(finalVert.value()));
        if (resources.Get<Rendering::Shader>(key)) {
            std::cerr << "[ProjectBrowser] Shader '" << key << "' already exists. Skipping.\n";
            return;
        }

        auto shader = resources.Load<Rendering::Shader>(key, finalVert.value(), finalFrag.value());
        Rendering::Renderer::SubmitInitTask([shader] { shader->InitGL(); });
        std::cout << "[ProjectBrowser] Imported shader '" << key << "'\n";
    }

    void ProjectBrowserPanel::ReplaceTexture(Core::ResourceManager &resources, const std::string &key) const {
        if (!m_EngineContext)
            return;

        const auto picked =
                FileDialogs::OpenFile(*m_EngineContext, {.filterName = "Image", .filterExt = "png,jpg,jpeg,bmp,tga"});
        if (!picked.has_value())
            return;

        auto finalPath = IO::AssetLoader::ImportAsset(picked.value(), "textures");
        if (!finalPath.has_value())
            return;

        resources.Unload<Rendering::Texture>(key);
        auto tex = resources.Load<Rendering::Texture>(key, finalPath.value());
        Rendering::Renderer::SubmitInitTask([tex] { tex->InitGL(); });
        std::cout << "[ProjectBrowser] Replaced texture '" << key << "'\n";
    }

    void ProjectBrowserPanel::ReplaceShader(Core::ResourceManager &resources, const std::string &key) const {
        if (!m_EngineContext)
            return;

        const auto vertPicked =
                FileDialogs::OpenFile(*m_EngineContext, {.filterName = "Vertex Shader", .filterExt = "vert,glsl"});
        if (!vertPicked.has_value())
            return;

        const auto fragPicked =
                FileDialogs::OpenFile(*m_EngineContext, {.filterName = "Fragment Shader", .filterExt = "frag,glsl"});
        if (!fragPicked.has_value())
            return;

        auto finalVert = IO::AssetLoader::ImportAsset(vertPicked.value(), "shaders");
        auto finalFrag = IO::AssetLoader::ImportAsset(fragPicked.value(), "shaders");
        if (!finalVert.has_value() || !finalFrag.has_value())
            return;

        resources.Unload<Rendering::Shader>(key);
        auto shader = resources.Load<Rendering::Shader>(key, finalVert.value(), finalFrag.value());
        Rendering::Renderer::SubmitInitTask([shader] { shader->InitGL(); });
        std::cout << "[ProjectBrowser] Replaced shader '" << key << "'\n";
    }

    void ProjectBrowserPanel::CreateMesh(Core::ResourceManager &resources) {
        const std::string id(m_MeshNameBuffer);
        if (id.empty())
            return;

        Rendering::MeshData data;
        constexpr const char *meshTypes[] = {"Quad",    "PointTopHex", "ETriang", "Ellipse", "Circle", "Pentagon",
                                             "Hexagon", "Octagon",     "Ring",    "Sector",  "Diamond"};
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

        auto mesh = resources.LoadFromFactory<Rendering::Mesh>(
                id, [data = std::move(data), meshTypes, factoryIdx = m_SelectedMeshFactory] {
                    auto m = std::make_shared<Rendering::Mesh>(std::move(data));
                    m->SetFactoryId(meshTypes[factoryIdx]);
                    return m;
                });

        Rendering::Renderer::SubmitInitTask([mesh] { mesh->InitGL(); });
        m_MeshNameBuffer[0] = '\0';
        std::cout << "[ProjectBrowser] Created mesh '" << id << "'\n";
    }

    void ProjectBrowserPanel::ImportFile(const std::string &targetSubDir, const char *filterExt,
                                         const char *filterName) const {
        if (!m_EngineContext)
            return;

        const auto picked = FileDialogs::OpenFile(*m_EngineContext, {.filterName = filterName, .filterExt = filterExt});
        if (!picked.has_value())
            return;

        IO::AssetLoader::ImportAsset(picked.value(), targetSubDir);
    }

} // namespace Editor::UI
