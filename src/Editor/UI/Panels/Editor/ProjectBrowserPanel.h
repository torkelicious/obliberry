#pragma once

#include "Editor/UI/Panels/EditorPanel.h"
#include "Core/ResourceManager.h"
#include <filesystem>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace Editor::UI {

    struct AssetEntry {
        std::string name;
        std::string virtualPath;
    };

    class ProjectBrowserPanel : public EditorPanel {
    public:
        void OnImGuiRender() override;

    private:
        enum class AssetType : uint8_t { Texture, Shader, Mesh, Material };

        void DrawTextureSection(Core::ResourceManager &resources);
        void DrawShaderSection(Core::ResourceManager &resources);
        void DrawMeshSection(Core::ResourceManager &resources);
        void DrawMaterialSection(Core::ResourceManager &resources);
        void DrawFileSection(const char *label, const std::string &directory, const std::string &extension,
                             const char *importFilter, const char *importFilterName);

        void ImportTexture(Core::ResourceManager &resources) const;
        void ImportShader(Core::ResourceManager &resources) const;
        void CreateMesh(Core::ResourceManager &resources);
        void ImportFile(const std::string &targetSubDir, const char *filterExt, const char *filterName) const;

        void ReplaceTexture(Core::ResourceManager &resources, const std::string &key) const;
        void ReplaceShader(Core::ResourceManager &resources, const std::string &key) const;

        void DrawDeleteConfirmPopup(Core::ResourceManager &resources);

        // UI state
        int m_SelectedMeshFactory = 0;
        char m_MeshNameBuffer[64] = {};
        char m_MaterialNameBuffer[64] = {};
        glm::vec4 m_MaterialColor = {1.0f, 1.0f, 1.0f, 1.0f};
        int m_SelectedMaterialShaderIdx = 0;
        int m_SelectedMaterialTextureIdx = 0;

        // rename state
        std::string m_RenamingKey;
        char m_RenameBuffer[128] = {};
        bool m_RenameJustActivated = false;

        // Delete confirmation state
        std::string m_DeleteConfirmKey;
        AssetType m_DeleteConfirmType;
        std::string m_DeleteConfirmFilePath; // for file based assets

        char m_NewScriptBuffer[64] = {};

        enum class ViewMode { Grid, List };
        ViewMode m_ViewMode = ViewMode::Grid;
        char m_SearchBuffer[128] = {};

        [[nodiscard]] std::vector<AssetEntry> ScanDirectory(const std::string &subDir,
                                                            const std::string &extension) const;

        [[nodiscard]] static std::string KeyFromPath(const std::filesystem::path &path);
    };

} // namespace Editor::UI
