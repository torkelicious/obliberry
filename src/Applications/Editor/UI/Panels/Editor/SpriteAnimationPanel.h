#pragma once
#include "Applications/Editor/UI/Panels/EditorPanel.h"
#include "ECS/Components/SpriteAnimatorComponent.h"
#include "ECS/Components/SpriteSheetComponent.h"
#include "ECS/Systems/Animation/Types.h"

namespace Editor::UI {

    class SpriteAnimationPanel : public EditorPanel {
    public:
        void Open(const std::string &key, const std::shared_ptr<Animation::SpriteAnimationSet> &asset);
        void Create(const std::string &key, const std::filesystem::path &path);

        void OnImGuiRender() override;

    private:
        void DrawSheetSettings();
        void DrawClipList();
        void DrawClipSettings();
        void DrawPreview();

        void Reset();
        void ResetPreview();
        bool Save();

        std::string m_AssetKey;
        std::string m_SelectedClip;
        std::string m_Status;

        char m_ClipNameBuffer[128] = {};

        std::shared_ptr<Animation::SpriteAnimationSet> m_Set;   // used
        std::shared_ptr<Animation::SpriteAnimationSet> m_Draft; // local

        bool m_Open = false;
        bool m_Dirty = false;

        // preveiw assets
        ECS::Components::SpriteAnimatorComponent m_PreviewPlayer;
        ECS::Components::SpriteSheetComponent m_PreveiwSprite;
    };

} // namespace Editor::UI
