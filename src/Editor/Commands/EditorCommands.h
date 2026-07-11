#pragma once
#include "ICommand.h"
#include "Core/ProjectConfig.h"

#include <glm/glm.hpp>
#include "ECS/Types.h"
#include "ECS/Components/ScriptComponent.h"
#include "Scenes/SceneManager.h"

namespace Editor::Commands {

    // = = = = = //
    // TRANSFORM //
    // = = = = = //

    //
    // Move
    //

    class TranslateEntityCommand final : public ICommand {
    public:
        // i could make this a unified transform command
        // but transforms are much larger than just the vec3's they hold sooo...
        // optimization ig :DDDD
        TranslateEntityCommand(ECS::EntityID target, glm::vec3 oldPos, glm::vec3 newPos);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        ECS::EntityID m_EntityID;
        glm::vec3 m_OldPos;
        glm::vec3 m_NewPos;
    };

    //
    // Rotate
    //

    class RotateEntityCommand final : public ICommand {
    public:
        RotateEntityCommand(ECS::EntityID target, glm::vec3 oldRot, glm::vec3 newRot);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        ECS::EntityID m_EntityID;
        glm::vec3 m_OldRot;
        glm::vec3 m_NewRot;
    };

    //
    // Scale
    //

    class ScaleEntityCommand final : public ICommand {
    public:
        ScaleEntityCommand(ECS::EntityID target, glm::vec3 oldScale, glm::vec3 newScale);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        ECS::EntityID m_EntityID;
        glm::vec3 m_OldScale;
        glm::vec3 m_NewScale;
    };

    // = = = =  //
    // Registry //
    // = = = =  //
    /* ( or scene in general ) */

    // Remove Component
    template <typename T> class RemoveComponentCommand final : public ICommand {
    public:
        RemoveComponentCommand(const ECS::EntityID target, const T &componentData) : m_EntityID(target), m_OldData(componentData) {}
        void Execute(Core::EngineContext &ctx) override { ctx.sceneManager->GetCurrentScene()->GetRegistry().RemoveComponent<T>(m_EntityID); }
        void Undo(Core::EngineContext &ctx) override { ctx.sceneManager->GetCurrentScene()->GetRegistry().AddComponent<T>(m_EntityID, m_OldData); }
        [[nodiscard]] std::string_view Name() const noexcept override { return "Remove Component"; }

    private:
        ECS::EntityID m_EntityID;
        T m_OldData;
    };

    // Add Component
    template <typename T> class AddComponentCommand final : public ICommand {
    public:
        AddComponentCommand(const ECS::EntityID target, const T &componentData) : m_EntityID(target), m_Data(componentData) {}
        void Execute(Core::EngineContext &ctx) override { ctx.sceneManager->GetCurrentScene()->GetRegistry().AddComponent<T>(m_EntityID, m_Data); }
        void Undo(Core::EngineContext &ctx) override { ctx.sceneManager->GetCurrentScene()->GetRegistry().RemoveComponent<T>(m_EntityID); }
        [[nodiscard]] std::string_view Name() const noexcept override { return "Add Component"; }

    private:
        ECS::EntityID m_EntityID;
        T m_Data;
    };


    // SCRIPT COMPONENT HAS ITS OWN HANDLER:
    class RemoveScriptCommand : public ICommand {
    public:
        RemoveScriptCommand(ECS::EntityID target, ECS::Components::ScriptComponent &component, int index);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        ECS::EntityID m_EntityID;
        ECS::Components::ScriptComponent *m_scriptComp = nullptr;
        int m_Index;
        bool m_ComponentRemoved = false;
        // per-entry data
        std::string m_SavedPath;
        std::vector<std::shared_ptr<ObSL::Environment>> m_SavedInstanceEnvs;
        bool m_IsInitialized = false;
        std::string m_SavedSourceCode;
        std::filesystem::file_time_type m_SavedLastModified;
    };


    class AddScriptCommand : public ICommand {
    public:
        AddScriptCommand(ECS::EntityID target, const std::string &script_path);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        ECS::EntityID m_EntityID;
        ECS::Components::ScriptComponent *m_Comp = nullptr;
        std::string m_PendingPath;
    };


    // TODO:
    //  Entity Deletion
    //  veri hard because ecs purges dead entities -.-

    // = = = = //
    // Project //
    // = = = = //

    //
    // Project config
    //
    class ProjectConfigUpdateCommand : public ICommand {
    public:
        ProjectConfigUpdateCommand(const Core::ProjectConfig &oldCfg, const Core::ProjectConfig &newCfg);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        Core::ProjectConfig m_OldData;
        Core::ProjectConfig m_NewData;
    };

    // = = //
    // Map //
    // = = //


} // namespace Editor::Commands
