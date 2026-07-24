#include "EditorCommands.h"
#include "Core/Project.h"
#include "ECS/Entity.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MapComponent.h"
#include "Applications/Editor/UI/Panels/Editor/EditorWidgets.h"
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

    //
    // Rename Entity
    //

    SetNameCommand::SetNameCommand(const ECS::EntityID target, std::string oldName, std::string newName) : m_EntityID(target), m_OldName(std::move(oldName)), m_NewName(std::move(newName)) {}

    void SetNameCommand::Execute(Core::EngineContext &ctx) { ctx.sceneManager->GetCurrentScene()->GetRegistry().SetEntityName(m_EntityID, m_NewName); }

    void SetNameCommand::Undo(Core::EngineContext &ctx) { ctx.sceneManager->GetCurrentScene()->GetRegistry().SetEntityName(m_EntityID, m_OldName); }

    std::string_view SetNameCommand::Name() const noexcept { return "Rename entity"; }

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
    AddScriptCommand::AddScriptCommand(const ECS::EntityID target, const std::string &script_path) : m_EntityID(target), m_PendingPath(script_path) {}

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
            if (auto *mapComp = ctx.sceneManager->GetCurrentScene()->GetRegistry().GetFirst<ECS::Components::MapComponent>())
                mapComp->lightmap.ambient = m_NewData.AmbientLight;
        }
        if (ctx.renderer) {
            Rendering::Renderer::SetClearColor(m_NewData.BackgroundClearColor);
        }
        RefreshWindowTitle(ctx);
    }

    void UpdateScenePropertiesCommand::Undo(Core::EngineContext &ctx) {
        if (ctx.sceneManager->GetCurrentScene()) {
            ctx.sceneManager->GetCurrentScene()->GetProperties() = m_OldData;
            ctx.sceneManager->GetCurrentScene()->MarkAsChanged();
            if (auto *mapComp = ctx.sceneManager->GetCurrentScene()->GetRegistry().GetFirst<ECS::Components::MapComponent>())
                mapComp->lightmap.ambient = m_OldData.AmbientLight;
        }
        if (ctx.renderer) {
            Rendering::Renderer::SetClearColor(m_OldData.BackgroundClearColor);
        }
        RefreshWindowTitle(ctx);
    }

    std::string_view UpdateScenePropertiesCommand::Name() const noexcept { return "Scene properties change"; }


    //
    // Project
    //
    ProjectConfigUpdateCommand::ProjectConfigUpdateCommand(const Config::ProjectConfig &oldCfg, const Config::ProjectConfig &newCfg) : m_OldData(oldCfg), m_NewData(newCfg) {}
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

    void MapChangeTileCommand::ApplyStates(const StateMap &states) const {
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


    //
    // Graphics Config
    //
    GraphicsConfigUpdateCommand::GraphicsConfigUpdateCommand(const Config::GraphicsConfig &oldCfg, const Config::GraphicsConfig &newCfg) : m_OldData(oldCfg), m_NewData(newCfg) {}

    void GraphicsConfigUpdateCommand::Execute(Core::EngineContext &ctx) {
        if (ctx.graphicsConfig)
            *ctx.graphicsConfig = m_NewData;
    }
    void GraphicsConfigUpdateCommand::Undo(Core::EngineContext &ctx) {
        if (ctx.graphicsConfig)
            *ctx.graphicsConfig = m_OldData;
    }

    std::string_view GraphicsConfigUpdateCommand::Name() const noexcept { return "Graphics settings change"; }

    // = = = = = //
    //   UI    //
    // = = = = = //

    UIElementSnapshot SnapshotUIElement(const ::UI::UIElement *element) {
        UIElementSnapshot snap;
        if (!element)
            return snap;

        snap.name = element->Name;
        snap.position = element->Rect.Position;
        snap.scale = element->Rect.Scale;
        snap.flags = element->HasFlag(::UI::VISIBLE) ? (snap.flags | ::UI::VISIBLE) : (snap.flags & ~::UI::VISIBLE);
        snap.flags = element->HasFlag(::UI::ENABLED) ? (snap.flags | ::UI::ENABLED) : (snap.flags & ~::UI::ENABLED);

        if (const auto *rect = dynamic_cast<const ::UI::UIRect *>(element)) {
            snap.type = UIElementSnapshot::RECT;
            snap.color = rect->GetColor();
        } else if (const auto *text = dynamic_cast<const ::UI::UIText *>(element)) {
            snap.type = UIElementSnapshot::TEXT;
            snap.text = text->GetText();
            snap.font = text->GetFont();
            snap.color = text->GetColor();
        } else if (const auto *btn = dynamic_cast<const ::UI::UIButton *>(element)) {
            snap.type = UIElementSnapshot::BUTTON;
            snap.text = btn->GetText();
            snap.font = btn->GetFont();
            snap.color = btn->GetColor();
            snap.bgColor = btn->GetBackgroundColor();
            snap.hoverBgColor = btn->GetHoveredBackgroundColor();
            snap.bgTexture = btn->GetBackgroundTexture();
        } else if (const auto *img = dynamic_cast<const ::UI::UIImage *>(element)) {
            snap.type = UIElementSnapshot::IMAGE;
            snap.color = img->GetColor();
            snap.image = const_cast<::UI::UIImage *>(img)->GetImage();
        }

        return snap;
    }

    std::unique_ptr<::UI::UIElement> CreateUIElementFromSnapshot(const UIElementSnapshot &snap) {
        std::unique_ptr<::UI::UIElement> el;

        switch (snap.type) {
            case UIElementSnapshot::RECT: {
                auto r = std::make_unique<::UI::UIRect>();
                r->SetColor(snap.color);
                el = std::move(r);
                break;
            }
            case UIElementSnapshot::TEXT: {
                auto t = std::make_unique<::UI::UIText>();
                t->SetText(snap.text);
                if (snap.font)
                    t->SetFont(snap.font);
                t->SetColor(snap.color);
                el = std::move(t);
                break;
            }
            case UIElementSnapshot::BUTTON: {
                auto b = std::make_unique<::UI::UIButton>();
                b->SetText(snap.text);
                if (snap.font)
                    b->SetFont(snap.font);
                b->SetColor(snap.color);
                b->SetBackgroundColor(snap.bgColor);
                b->SetHoveredBackgroundColor(snap.hoverBgColor);
                if (snap.bgTexture)
                    b->SetBackgroundTexture(snap.bgTexture);
                el = std::move(b);
                break;
            }
            case UIElementSnapshot::IMAGE: {
                auto i = std::make_unique<::UI::UIImage>();
                if (snap.image)
                    i->SetImage(snap.image);
                i->SetColor(snap.color);
                el = std::move(i);
                break;
            }
        }

        if (el) {
            el->Name = snap.name;
            el->Rect.Position = snap.position;
            el->Rect.Scale = snap.scale;
            if (snap.flags & ::UI::VISIBLE)
                el->AddFlag(::UI::VISIBLE);
            else
                el->RemoveFlag(::UI::VISIBLE);
            if (snap.flags & ::UI::ENABLED)
                el->AddFlag(::UI::ENABLED);
            else
                el->RemoveFlag(::UI::ENABLED);
        }

        return el;
    }

    // AddUIElementCommand
    AddUIElementCommand::AddUIElementCommand(::UI::UISystem *sys, ::UI::UIElement *parent, UIElementSnapshot snapshot) : m_UISystem(sys), m_Parent(parent), m_Snapshot(std::move(snapshot)) {}

    void AddUIElementCommand::Execute(Core::EngineContext &ctx) {
        auto el = CreateUIElementFromSnapshot(m_Snapshot);
        if (el && m_UISystem && m_Parent) {
            m_Created = m_UISystem->AddChild(m_Parent, std::move(el));
        }
    }

    void AddUIElementCommand::Undo(Core::EngineContext &ctx) {
        if (m_Created && m_UISystem && m_Parent) {
            m_UISystem->RemoveChild(m_Parent, m_Created);
            m_Created = nullptr;
        }
    }

    // RemoveUIElementCommand
    RemoveUIElementCommand::RemoveUIElementCommand(::UI::UISystem *sys, ::UI::UIElement *parent, ::UI::UIElement *child) : m_UISystem(sys), m_Parent(parent), m_Snapshot(SnapshotUIElement(child)) {}

    void RemoveUIElementCommand::Execute(Core::EngineContext &ctx) {
        if (m_Restored && m_UISystem && m_Parent) {
            // Re-remove on redo
            m_UISystem->RemoveChild(m_Parent, m_Restored);
            m_Restored = nullptr;
        } else if (!m_Snapshot.name.empty() && m_UISystem && m_Parent) {
            // First time: find the child by matching snapshot data
            for (auto *child : m_Parent->Children) {
                if (child->Name == m_Snapshot.name && child->Rect.Position == m_Snapshot.position) {
                    m_UISystem->RemoveChild(m_Parent, child);
                    break;
                }
            }
        }
    }

    void RemoveUIElementCommand::Undo(Core::EngineContext &ctx) {
        auto el = CreateUIElementFromSnapshot(m_Snapshot);
        if (el && m_UISystem && m_Parent) {
            m_Restored = m_UISystem->AddChild(m_Parent, std::move(el));
        }
    }

} // namespace Editor::Commands
