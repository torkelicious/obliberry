#pragma once

#include "Applications/Editor/UI/Panels/EditorPanel.h"
#include "Core/ResourceManager.h"
#include "ECS/Systems/Animation/Types.h"
#include "Scripting/SmallFunction.h"
#include <filesystem>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <type_traits>

namespace Editor::UI {

    struct AssetEntry {
        std::string name;
        std::string virtualPath;
    };

    class ProjectBrowserPanel : public EditorPanel {
    public:
        void OnImGuiRender() override;

        Scripting::SmallFunction<void(const std::string &, const std::shared_ptr<Animation::SpriteAnimationSet> &)> OnEditAnimation;

        Scripting::SmallFunction<void(const std::string &, const std::filesystem::path &)> OnCreateAnimation;

    private:
        enum class AssetType : uint8_t { Texture, Shader, Mesh, Material, Font, Animation };

        template <typename T>
        void DrawResourceSection(Core::ResourceManager &resources, const std::vector<std::pair<std::string, std::shared_ptr<T>>> &allItems, AssetType assetType, const char *childId, float childHeight,
                const char *emptyText, const char *typeName, std::type_identity_t<std::function<void(const std::shared_ptr<T> &)>> renderThumbnail,
                const std::type_identity_t<std::function<void(const std::string &, Core::ResourceManager &)>> &renderExtraButtons,
                std::type_identity_t<std::function<void(const std::string &, const std::shared_ptr<T> &, Core::ResourceManager &)>> renderTooltip = nullptr);

        void DrawTextureSection(Core::ResourceManager &resources);
        void DrawShaderSection(Core::ResourceManager &resources);
        void DrawMeshSection(Core::ResourceManager &resources);
        void DrawMaterialSection(Core::ResourceManager &resources);
        void DrawFontSection(Core::ResourceManager &resources);
        void DrawFileSection(const char *label, const std::string &directory, const std::string &extension, const char *importFilter, const char *importFilterName);
        void DrawAnimationSelection();
        void DrawCreateAnimation();


        void ImportTexture() const;
        void ImportShader() const;
        void ImportFont() const;
        void ImportAnimation() const;

        void CreateMesh();
        void ImportFile(const std::string &targetSubDir, const char *filterExt, const char *filterName) const;

        void ReplaceTexture(const std::string &key) const;
        void ReplaceShader(const std::string &key) const;
        void ReplaceFont(const std::string &key) const;

        void DrawDeleteConfirmPopup(Core::ResourceManager &resources);

        // UI state
        int m_SelectedMeshFactory = 0;
        char m_MeshNameBuffer[64] = {};
        char m_MaterialNameBuffer[64] = {};
        glm::vec4 m_MaterialColor = {1.0f, 1.0f, 1.0f, 1.0f};
        int m_SelectedMaterialShaderIdx = 0;
        int m_SelectedMaterialTextureIdx = 0;

        char m_NewAnimationID[128] = {};
        std::string m_CreateAnimationError;

        int m_FontSize = 48;
        bool m_FontUseSDF = false;
        int m_FontSDFSpread = 8;

        // Delete confirmation state
        std::string m_DeleteConfirmKey;
        AssetType m_DeleteConfirmType{};
        std::string m_DeleteConfirmFilePath; // for file based assets

        char m_NewScriptBuffer[64] = {};

        enum class ViewMode { Grid, List };

        ViewMode m_ViewMode = ViewMode::Grid;
        char m_SearchBuffer[128] = {};

        struct DirScanCache {
            std::string dirKey;
            std::filesystem::file_time_type lastWrite{};
            bool valid = false;
            std::vector<AssetEntry> entries;
        };
        std::unordered_map<std::string, DirScanCache> m_ScanCaches;

        [[nodiscard]] std::vector<AssetEntry> ScanDirectoryCached(const std::string &subDir, const std::string &extension);
        [[nodiscard]] static std::vector<AssetEntry> ScanDirectory(const std::string &subDir, const std::string &extension);

        [[nodiscard]] static std::string KeyFromPath(const std::filesystem::path &path);
    };

} // namespace Editor::UI
