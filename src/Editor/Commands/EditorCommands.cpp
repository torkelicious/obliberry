#include "EditorCommands.h"

#include "Core/Project.h"
#include "Core/Window.h"
#include "ECS/Entity.h"
#include "ECS/Components/TransformComponent.h"
#include "Editor/UI/Panels/Editor/EditorWidgets.h"
#include "Rendering/Renderer.h"
#include "Scenes/SceneManager.h"
#include "Sound/AudioEngine.h"
namespace Editor::Commands {

    //
    // Transforms
    //

    // Move Transform
    TranslateEntityCommand::TranslateEntityCommand(const ECS::EntityID target, const glm::vec3 oldPos, const glm::vec3 newPos) : m_EntityID(target), m_OldPos(oldPos), m_NewPos(newPos) {}

    void TranslateEntityCommand::Execute(Core::EngineContext &ctx) {
        const ECS::Entity ent(m_EntityID, &ctx.sceneManager->GetCurrentScene()->GetRegistry());
        ent.GetComponent<ECS::Components::TransformComponent>()->transform.SetPosition(m_NewPos);
    }

    void TranslateEntityCommand::Undo(Core::EngineContext &ctx) {
        const ECS::Entity ent(m_EntityID, &ctx.sceneManager->GetCurrentScene()->GetRegistry());
        ent.GetComponent<ECS::Components::TransformComponent>()->transform.SetPosition(m_OldPos);
    }
    std::string_view TranslateEntityCommand::Name() const noexcept { return "Move entity"; }

    // Rotate Transform
    RotateEntityCommand::RotateEntityCommand(const ECS::EntityID target, const glm::vec3 oldRot, const glm::vec3 newRot) : m_EntityID(target), m_OldRot(oldRot), m_NewRot(newRot) {}
    void RotateEntityCommand::Execute(Core::EngineContext &ctx) {
        const ECS::Entity ent(m_EntityID, &ctx.sceneManager->GetCurrentScene()->GetRegistry());
        ent.GetComponent<ECS::Components::TransformComponent>()->transform.SetRotation(m_NewRot);
    }
    void RotateEntityCommand::Undo(Core::EngineContext &ctx) {
        const ECS::Entity ent(m_EntityID, &ctx.sceneManager->GetCurrentScene()->GetRegistry());
        ent.GetComponent<ECS::Components::TransformComponent>()->transform.SetRotation(m_OldRot);
    }
    std::string_view RotateEntityCommand::Name() const noexcept { return "Rotate entity"; }

    // Scale Transform
    ScaleEntityCommand::ScaleEntityCommand(const ECS::EntityID target, const glm::vec3 oldScale, const glm::vec3 newScale) : m_EntityID(target), m_OldScale(oldScale), m_NewScale(newScale) {}
    void ScaleEntityCommand::Execute(Core::EngineContext &ctx) {
        const ECS::Entity ent(m_EntityID, &ctx.sceneManager->GetCurrentScene()->GetRegistry());
        ent.GetComponent<ECS::Components::TransformComponent>()->transform.SetScale(m_NewScale);
    }
    void ScaleEntityCommand::Undo(Core::EngineContext &ctx) {
        const ECS::Entity ent(m_EntityID, &ctx.sceneManager->GetCurrentScene()->GetRegistry());
        ent.GetComponent<ECS::Components::TransformComponent>()->transform.SetScale(m_OldScale);
    }
    std::string_view ScaleEntityCommand::Name() const noexcept { return "Scale entity"; }

    // normal components commands are in the header (EditorCommands.h)

    //
    // Script Component
    //

    // Remove script
    RemoveScriptCommand::RemoveScriptCommand(const ECS::EntityID target, ECS::Components::ScriptComponent &component, const int index) : m_EntityID(target), m_scriptComp(&component), m_Index(index) {
        // save only copyable fields for the entry being removed
        m_SavedPath = component.scriptPaths[index];
        m_SavedInstanceEnvs = component.instance_envs[index];
        m_IsInitialized = component.isInitialized[index];
        m_SavedSourceCode = component.source_codes[index];
        m_SavedLastModified = component.lastModified[index];
    }
    void RemoveScriptCommand::Execute(Core::EngineContext &ctx) {
        m_scriptComp->scriptPaths.erase(m_scriptComp->scriptPaths.begin() + m_Index);
        m_scriptComp->instance_envs.erase(m_scriptComp->instance_envs.begin() + m_Index);
        m_scriptComp->on_update_functions.erase(m_scriptComp->on_update_functions.begin() + m_Index);
        m_scriptComp->on_destroy_functions.erase(m_scriptComp->on_destroy_functions.begin() + m_Index);
        m_scriptComp->on_exit_functions.erase(m_scriptComp->on_exit_functions.begin() + m_Index);
        m_scriptComp->isInitialized.erase(m_scriptComp->isInitialized.begin() + m_Index);
        m_scriptComp->source_codes.erase(m_scriptComp->source_codes.begin() + m_Index);
        m_scriptComp->ast_nodes.erase(m_scriptComp->ast_nodes.begin() + m_Index);
        m_scriptComp->lastModified.erase(m_scriptComp->lastModified.begin() + m_Index);
        if (m_scriptComp->scriptPaths.empty()) {
            ctx.sceneManager->GetCurrentScene()->GetRegistry().RemoveComponent<ECS::Components::ScriptComponent>(m_EntityID);
            m_scriptComp = nullptr;
            m_ComponentRemoved = true;
        }
    }
    void RemoveScriptCommand::Undo(Core::EngineContext &ctx) {
        auto &registry = ctx.sceneManager->GetCurrentScene()->GetRegistry();
        if (m_ComponentRemoved) {
            m_scriptComp = &registry.AddComponent<ECS::Components::ScriptComponent>(m_EntityID);
            m_ComponentRemoved = false;
        }
        // insert saved data back at the original index
        m_scriptComp->scriptPaths.insert(m_scriptComp->scriptPaths.begin() + m_Index, m_SavedPath);
        m_scriptComp->instance_envs.insert(m_scriptComp->instance_envs.begin() + m_Index, m_SavedInstanceEnvs);
        m_scriptComp->on_update_functions.insert(m_scriptComp->on_update_functions.begin() + m_Index, {});
        m_scriptComp->on_destroy_functions.insert(m_scriptComp->on_destroy_functions.begin() + m_Index, {});
        m_scriptComp->on_exit_functions.insert(m_scriptComp->on_exit_functions.begin() + m_Index, {});
        m_scriptComp->isInitialized.insert(m_scriptComp->isInitialized.begin() + m_Index, m_IsInitialized);
        m_scriptComp->source_codes.insert(m_scriptComp->source_codes.begin() + m_Index, m_SavedSourceCode);
        m_scriptComp->ast_nodes.emplace(m_scriptComp->ast_nodes.begin() + m_Index);
        m_scriptComp->lastModified.insert(m_scriptComp->lastModified.begin() + m_Index, m_SavedLastModified);
    }
    std::string_view RemoveScriptCommand::Name() const noexcept { return "Remove Script"; }

    // Add script
    AddScriptCommand::AddScriptCommand(ECS::EntityID target, const std::string &script_path) : m_EntityID(target), m_PendingPath(script_path) {}

    void AddScriptCommand::Execute(Core::EngineContext &ctx) {
        if (!m_PendingPath.empty()) {
            auto &registry = ctx.sceneManager->GetCurrentScene()->GetRegistry();
            if (!registry.HasComponent<ECS::Components::ScriptComponent>(m_EntityID)) {
                m_Comp = &registry.AddComponent<ECS::Components::ScriptComponent>(m_EntityID);
            }
            if (!m_Comp)
                m_Comp = registry.GetComponent<ECS::Components::ScriptComponent>(m_EntityID);
            m_Comp->scriptPaths.push_back(m_PendingPath);
            m_Comp->instance_envs.emplace_back();
            m_Comp->on_update_functions.emplace_back();
            m_Comp->on_destroy_functions.emplace_back();
            m_Comp->on_exit_functions.emplace_back();
            m_Comp->isInitialized.push_back(false);
            m_Comp->source_codes.emplace_back();
            m_Comp->ast_nodes.emplace_back();
            m_Comp->lastModified.push_back(std::filesystem::file_time_type::min());
            UI::MarkSceneChanged(&ctx);
        }
    }

    void AddScriptCommand::Undo(Core::EngineContext &ctx) {
        if (!m_Comp)
            return;
        auto &registry = ctx.sceneManager->GetCurrentScene()->GetRegistry();
        m_Comp = registry.GetComponent<ECS::Components::ScriptComponent>(m_EntityID);
        if (!m_Comp)
            return;
        m_Comp->scriptPaths.pop_back();
        m_Comp->instance_envs.pop_back();
        m_Comp->on_update_functions.pop_back();
        m_Comp->on_destroy_functions.pop_back();
        m_Comp->on_exit_functions.pop_back();
        m_Comp->isInitialized.pop_back();
        m_Comp->source_codes.pop_back();
        m_Comp->ast_nodes.pop_back();
        m_Comp->lastModified.pop_back();
        UI::MarkSceneChanged(&ctx);
    }

    std::string_view AddScriptCommand::Name() const noexcept { return "Add script"; }

    // Scene Properties
    UpdateScenePropertiesCommand::UpdateScenePropertiesCommand(const Scenes::SceneProperties &oldCfg, const Scenes::SceneProperties &newCfg) : m_OldData(oldCfg), m_NewData(newCfg) {}
    void UpdateScenePropertiesCommand::Execute(Core::EngineContext &ctx) {
        if (ctx.sceneManager->GetCurrentScene()) {
            ctx.sceneManager->GetCurrentScene()->GetProperties() = m_NewData;
            ctx.sceneManager->GetCurrentScene()->MarkAsChanged();
        }
        if (ctx.renderer) {
            Rendering::Renderer::SetClearColor(m_NewData.BackgroundClearColor);
        }
        if (ctx.audioEngine) {
            if (!m_NewData.BackgroundMusicPath.empty()) {
                ctx.audioEngine->PlayMusic(m_NewData.BackgroundMusicPath);
            } else {
                ctx.audioEngine->StopMusic();
            }
        }
    }

    void UpdateScenePropertiesCommand::Undo(Core::EngineContext &ctx) {
        if (ctx.sceneManager->GetCurrentScene()) {
            ctx.sceneManager->GetCurrentScene()->GetProperties() = m_OldData;
            ctx.sceneManager->GetCurrentScene()->MarkAsChanged();
        }
        if (ctx.renderer) {
            Rendering::Renderer::SetClearColor(m_OldData.BackgroundClearColor);
        }
        if (ctx.audioEngine) {
            if (!m_OldData.BackgroundMusicPath.empty()) {
                ctx.audioEngine->PlayMusic(m_OldData.BackgroundMusicPath);
            } else {
                ctx.audioEngine->StopMusic();
            }
        }
    }

    std::string_view UpdateScenePropertiesCommand::Name() const noexcept { return "Scene properties change"; }


    //
    // Project
    //
    ProjectConfigUpdateCommand::ProjectConfigUpdateCommand(const Core::ProjectConfig &oldCfg, const Core::ProjectConfig &newCfg) : m_OldData(oldCfg), m_NewData(newCfg) {}
    // a lil hacky.. but it works :)
    static void RefreshWindowTitle(Core::EngineContext &ctx) {
        if (!ctx.window || !ctx.projectConfig)
            return;
        std::string title = "Obliberry: " + ctx.projectConfig->Title;
        if (ctx.sceneManager)
            if (auto *scene = ctx.sceneManager->GetCurrentScene())
                title += " - Scene - " + scene->GetProperties().ScenePath;
        ctx.window->SetWindowTitle(title);
    }

    void ProjectConfigUpdateCommand::Execute(Core::EngineContext &ctx) {
        if (ctx.projectConfig)
            *ctx.projectConfig = m_NewData;
        if (ctx.activeProject)
            ctx.activeProject->MarkAsChanged();
        RefreshWindowTitle(ctx);
    }
    void ProjectConfigUpdateCommand::Undo(Core::EngineContext &ctx) {
        if (ctx.projectConfig)
            *ctx.projectConfig = m_OldData;
        if (ctx.activeProject)
            ctx.activeProject->MarkAsChanged();
        RefreshWindowTitle(ctx);
    }
    std::string_view ProjectConfigUpdateCommand::Name() const noexcept { return "Project Configuration change"; }

    //
    // Map edit
    //

    // Paint / Erase
    MapChangeTileCommand::MapChangeTileCommand(StateMap oldState, StateMap newState, Map::HexGrid *grid, bool *meshDirty)
        : m_OldState(std::move(oldState)), m_NewState(std::move(newState)), m_Grid(grid), m_MeshDirty(meshDirty) {}

    void MapChangeTileCommand::ApplyStates(const StateMap &states) {
        for (const auto &[pos, state] : states) {
            m_Grid->RemoveTileAt(pos);
            if (state) {
                m_Grid->EmplaceTile(pos, state->first, state->second);
            }
        }
    }

    void MapChangeTileCommand::Execute(Core::EngineContext &ctx) {
        ApplyStates(m_NewState);
        if (m_MeshDirty)
            *m_MeshDirty = true;
    }

    void MapChangeTileCommand::Undo(Core::EngineContext &ctx) {
        ApplyStates(m_OldState);
        if (m_MeshDirty)
            *m_MeshDirty = true;
    }
    std::string_view MapChangeTileCommand::Name() const noexcept { return "Map paint / erase"; }


} // namespace Editor::Commands
