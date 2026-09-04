#pragma once


#include <string>
#include <glm/glm.hpp>
#include "UI/Rendering/UISystem.h"
#include "Core/EngineContext.h"
#include "ECS/ECS.h"
#include "ECS/Components/MapComponent.h"
#include "Rendering/PostProcessing/PostProcessing.h"
#include "Scripting/EngineLib/UICommandBuffer.h"

namespace Scenes {
    struct SceneProperties {
        std::string ScenePath;
        std::string Name;
        std::string BackgroundMusicPath;
        glm::vec4 BackgroundClearColor = {0, 0, 0, 1};
        float AmbientLight = 0.2f;
        bool EnableLightingSystem = true;
    };

    class Scene {
    public:
        ~Scene() = default;

        Scene(Core::EngineContext *context, SceneProperties props);

        void OnEnter();

        void Update(float dt);

        void Render();

        void OnExit();

        [[nodiscard]] ECS::Registry &GetRegistry() { return m_Registry; }
        [[nodiscard]] const ECS::Registry &GetRegistry() const { return m_Registry; }

        [[nodiscard]] Core::EngineContext &GetContext() { return *m_Context; }
        [[nodiscard]] const Core::EngineContext &GetContext() const { return *m_Context; }

        [[nodiscard]] UI::UISystem &GetUISystem() { return m_UISystem; }
        [[nodiscard]] const UI::UISystem &GetUISystem() const { return m_UISystem; }

        [[nodiscard]] const std::string &GetScenePath() const { return m_Properties.ScenePath; }

        SceneProperties &GetProperties() { return m_Properties; }

        void MarkAsChanged() { m_HasUnsavedChanges = true; }
        [[nodiscard]] bool HasUnsavedChanges() const { return m_HasUnsavedChanges; }
        void ClearUnsavedChanges() { m_HasUnsavedChanges = false; }

        void OnSaved();

        ECS::Components::MapComponent *GetMapComp();

        [[nodiscard]] std::vector<Rendering::PostProcessing::PostEffect> &PostFx() { return m_PostFx; }
        [[nodiscard]] const std::vector<Rendering::PostProcessing::PostEffect> &PostFx() const { return m_PostFx; }

    private:
        SceneProperties m_Properties;
        Core::EngineContext *m_Context;
        ECS::Registry m_Registry;
        bool m_HasUnsavedChanges = false;
        UI::UISystem m_UISystem;
        Scripting::UICommandBuffer m_UICmdBuf;
        std::vector<Rendering::PostProcessing::PostEffect> m_PostFx;
    };
} // namespace Scenes
