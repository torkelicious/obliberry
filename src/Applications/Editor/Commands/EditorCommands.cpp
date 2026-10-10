#include "EditorCommands.h"
#include "Core/EngineContext.h"
#include "Core/Project.h"
#include "Core/ResourceManager.h"
#include "ECS/Components/CustomDataComponent.h"
#include "ECS/Components/DestroyTagComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Components/PersistentTagComponent.h"
#include "ECS/Components/PrefabSourceComponent.h"
#include "ECS/Components/RelationshipComponent.h"
#include "ECS/Entity.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MapComponent.h"
#include "Applications/Editor/UI/Panels/Editor/EditorWidgets.h"
#include "Applications/Editor/UI/Themeing/ThemeSerializer.h"
#include "ECS/Types.h"
#include "IO/Loaders/EntityFactory.h"
#include "Rendering/Renderer.h"
#include "Scenes/SceneManager.h"
#include "Core/Utils/ECSUtils.h"
#include "IO/Loaders/UISerializer.h"
#include "UI/Elements/UIRect.h"
#include "UI/Elements/UIText.h"
#include "UI/Elements/UIImage.h"
#include <cstddef>
#include <unordered_set>
#include <vector>

namespace Editor::Commands {

    namespace {
        std::vector<DeletedEntitySnapshot> CaptureSubtree(ECS::Registry &registry, const ECS::EntityID root) {
            IO::EntityFactory::RegisterSerializers();
            std::vector<DeletedEntitySnapshot> snapshots;
            std::unordered_set<ECS::EntityID> visited;

            const auto capture = [&](auto &self, const ECS::EntityID id) -> void {
                if (!registry.IsValid(id) || !visited.insert(id).second) {
                    return;
                }
                DeletedEntitySnapshot snapshot;
                snapshot.uuid = registry.GetEntityUUID(id);
                snapshot.entityData = {{"components", nlohmann::json::object()}};

                ECS::Entity entity(id, &registry);
                IO::EntityFactory::SerializeEntity(entity, snapshot.entityData, Core::ResourceManager::GetInstance());
                snapshot.entityData["name"] = entity.GetName();

                auto &metadata = snapshot.entityData["editor_metadata"];
                metadata["persistent"] = registry.HasComponent<ECS::Components::PersistentTagComponent>(id);

                if (const auto *prefab = registry.GetComponent<ECS::Components::PrefabSourceComponent>(id)) {
                    metadata["prefab_source"] = {{"prefabPath", prefab->prefabPath}, {"originalData", prefab->originalData}};
                }

                std::vector<ECS::EntityID> children;

                if (const auto *relationship = registry.GetComponent<ECS::Components::RelationshipComponent>(id)) {
                    children = relationship->children;

                    const auto parent = relationship->parent;
                    if (registry.IsValid(parent)) {
                        snapshot.parentUUID = registry.GetEntityUUID(parent);

                        if (const auto *parentRelationship = registry.GetComponent<ECS::Components::RelationshipComponent>(parent)) {
                            const auto &siblings = parentRelationship->children;
                            const auto found = std::find(siblings.begin(), siblings.end(), id);
                            if (found != siblings.end()) {
                                snapshot.siblingIndex = static_cast<std::size_t>(found - siblings.begin());
                            }
                        }
                    }
                }
                snapshots.push_back(std::move(snapshot));

                for (const auto child : children) {
                    self(self, child);
                }
            };
            capture(capture, root);
            return snapshots;
        }

    } // namespace


    //
    // Transforms
    //

    // Move Transform
    TranslateEntityCommand::TranslateEntityCommand(std::string targetUUID, const glm::vec3 oldPos, const glm::vec3 newPos) : m_EntityUUID(std::move(targetUUID)), m_OldPos(oldPos), m_NewPos(newPos) {}

    void TranslateEntityCommand::SetPosition(Core::EngineContext &ctx, const glm::vec3 &position) {
        auto *scene = ctx.sceneManager ? ctx.sceneManager->GetCurrentScene() : nullptr;

        if (!scene) {
            return;
        }

        auto &registry = scene->GetRegistry();
        const auto id = registry.FindEntityByUUID(m_EntityUUID);

        if (auto *transform = registry.GetComponent<ECS::Components::TransformComponent>(id)) {
            transform->transform.SetPosition(position);
        }
    }

    void TranslateEntityCommand::Execute(Core::EngineContext &ctx) { SetPosition(ctx, m_NewPos); }

    void TranslateEntityCommand::Undo(Core::EngineContext &ctx) { SetPosition(ctx, m_OldPos); }

    std::string_view TranslateEntityCommand::Name() const noexcept { return "Move entity"; }

    // Rotate Transform
    RotateEntityCommand::RotateEntityCommand(const ECS::EntityID target, const glm::vec3 oldRot, const glm::vec3 newRot) : m_EntityID(target), m_OldRot(oldRot), m_NewRot(newRot) {}

    void RotateEntityCommand::Execute(Core::EngineContext &ctx) {
        const ECS::Entity ent(m_EntityID, &ctx.sceneManager->GetCurrentScene()->GetRegistry());
        if (auto *transform = ent.GetComponent<ECS::Components::TransformComponent>()) {
            transform->transform.SetRotation(m_NewRot);
        }
    }

    void RotateEntityCommand::Undo(Core::EngineContext &ctx) {
        const ECS::Entity ent(m_EntityID, &ctx.sceneManager->GetCurrentScene()->GetRegistry());
        if (auto *transform = ent.GetComponent<ECS::Components::TransformComponent>()) {
            transform->transform.SetRotation(m_OldRot);
        }
    }

    std::string_view RotateEntityCommand::Name() const noexcept { return "Rotate entity"; }

    // Scale Transform
    ScaleEntityCommand::ScaleEntityCommand(const ECS::EntityID target, const glm::vec3 oldScale, const glm::vec3 newScale) : m_EntityID(target), m_OldScale(oldScale), m_NewScale(newScale) {}

    void ScaleEntityCommand::Execute(Core::EngineContext &ctx) {
        const ECS::Entity ent(m_EntityID, &ctx.sceneManager->GetCurrentScene()->GetRegistry());
        if (auto *transform = ent.GetComponent<ECS::Components::TransformComponent>()) {
            transform->transform.SetScale(m_NewScale);
        }
    }

    void ScaleEntityCommand::Undo(Core::EngineContext &ctx) {
        const ECS::Entity ent(m_EntityID, &ctx.sceneManager->GetCurrentScene()->GetRegistry());
        if (auto *transform = ent.GetComponent<ECS::Components::TransformComponent>()) {
            transform->transform.SetScale(m_OldScale);
        }
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
        m_SavedPath = component.slots[index].scriptPath;
        m_SavedInstanceEnvs = component.slots[index].instance_envs;
        m_IsInitialized = component.slots[index].isInitialized;
        m_SavedSourceCode = component.slots[index].source_code;
        m_SavedLastModified = component.slots[index].lastModified;
    }

    void RemoveScriptCommand::Execute(Core::EngineContext &ctx) {
        m_scriptComp->slots.erase(m_scriptComp->slots.begin() + m_Index);
        if (m_scriptComp->slots.empty()) {
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
        ECS::Components::ScriptSlot slot;
        slot.scriptPath = m_SavedPath;
        slot.instance_envs = m_SavedInstanceEnvs;
        slot.isInitialized = m_IsInitialized;
        slot.source_code = m_SavedSourceCode;
        slot.lastModified = m_SavedLastModified;
        m_scriptComp->slots.insert(m_scriptComp->slots.begin() + m_Index, std::move(slot));
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
            ECS::Components::ScriptSlot slot;
            slot.scriptPath = m_PendingPath;
            m_Comp->slots.push_back(std::move(slot));
            MarkSceneChanged(&ctx);
        }
    }

    void AddScriptCommand::Undo(Core::EngineContext &ctx) {
        if (!m_Comp)
            return;
        auto &registry = ctx.sceneManager->GetCurrentScene()->GetRegistry();
        m_Comp = registry.GetComponent<ECS::Components::ScriptComponent>(m_EntityID);
        if (!m_Comp)
            return;
        m_Comp->slots.pop_back();
        MarkSceneChanged(&ctx);
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
    // Delete
    //
    DeleteEntityCommand::DeleteEntityCommand(std::string targetUUID) : m_RootUUID(std::move(targetUUID)) {}

    void DeleteEntityCommand::Execute(Core::EngineContext &ctx) {
        auto *scene = ctx.sceneManager->GetCurrentScene();

        if (!scene || !ctx.resources) {
            return;
        }

        auto &registry = scene->GetRegistry();
        const auto root = registry.FindEntityByUUID(m_RootUUID);

        if (!registry.IsValid(root)) {
            return;
        }

        if (m_Snapshot.empty()) {
            auto snapshot = CaptureSubtree(registry, root);

            for (const auto &saved : snapshot) {
                const auto id = registry.FindEntityByUUID(saved.uuid);
                if (registry.HasComponent<ECS::Components::MapComponent>(id) || registry.HasComponent<ECS::Components::MapStateComponent>(id) || registry.HasComponent<ECS::Components::CustomDataComponent>(id) ||
                        registry.HasComponent<ECS::Components::DestroyTagComponent>(id)) {
                    LOG_ERROR("EditorCommands", "Cannot delete subtree, unsupported snapshot component.");
                    return;
                }
            }

            if (snapshot.empty()) {
                return;
            }
            m_Snapshot = std::move(snapshot);
        }

        registry.DestroyEntity(root);
        MarkSceneChanged(&ctx);
        m_Succeeded = true;
    }

    void DeleteEntityCommand::Undo(Core::EngineContext &ctx) {
        auto *scene = ctx.sceneManager->GetCurrentScene();

        if (!scene || !ctx.resources || m_Snapshot.empty()) {
            return;
        }

        auto &registry = scene->GetRegistry();

        // deduplicate
        for (const auto &saved : m_Snapshot) {
            if (registry.IsValid(registry.FindEntityByUUID(saved.uuid))) {
                LOG_ERROR("EditorCommands", "Cannot restore subtree: an entity UUID already exists.");
                return;
            }
        }

        nlohmann::json references = nlohmann::json::object();
        references["entities"] = nlohmann::json::array();

        for (const auto &saved : m_Snapshot) {
            references["entities"].push_back(saved.entityData);
        }

        IO::SceneAssetLoader::SceneAssetScope assets;
        if (!IO::SceneAssetLoader::LoadReferenced(references, assets)) {
            LOG_ERROR("EditorCommands", "Cannot restore subtree: referenced assets could not be acquired.");
            return;
        }

        IO::EntityFactory::RegisterDeserializers();

        std::vector<ECS::EntityID> created;
        created.reserve(m_Snapshot.size());

        try {
            for (const auto &saved : m_Snapshot) {
                const auto id = registry.CreateEntity();

                if (!registry.IsValid(id)) {
                    throw std::runtime_error("Could not create restored entity.");
                }

                created.push_back(id);
                ECS::Entity entity(id, &registry);

                IO::EntityFactory::DeserializeEntity(entity, saved.entityData, *ctx.resources, true);

                if (registry.GetEntityUUID(id) != saved.uuid) {
                    throw std::runtime_error("Could not restore entity UUID.");
                }

                entity.SetName(saved.entityData.value("name", std::string{}));
                const auto metadata = saved.entityData.find("editor_metadata");

                if (metadata != saved.entityData.end()) {
                    if (metadata->value("persistent", false)) {
                        registry.AddComponent<ECS::Components::PersistentTagComponent>(id);
                    }
                    const auto prefab = metadata->find("prefab_source");
                    if (prefab != metadata->end()) {
                        registry.AddComponent<ECS::Components::PrefabSourceComponent>(id, prefab->at("prefabPath").get<std::string>(), prefab->at("originalData"));
                    }
                }
            }

            for (std::size_t i = 0; i < m_Snapshot.size(); ++i) {
                const auto &saved = m_Snapshot[i];

                if (saved.parentUUID.empty()) {
                    continue;
                }

                const auto parent = registry.FindEntityByUUID(saved.parentUUID);

                if (!registry.IsValid(parent)) {
                    throw std::runtime_error("Could not find the restored entity's parent.");
                }

                const auto id = created[i];
                registry.SetParentDirect(id, parent);

                auto &siblings = registry.GetComponent<ECS::Components::RelationshipComponent>(parent)->children;

                std::erase(siblings, id);

                const auto position = std::min(saved.siblingIndex, siblings.size());

                siblings.insert(siblings.begin() + position, id);
            }

            scene->AddAssetScope(std::move(assets));
        } catch (const std::exception &error) {
            for (auto it = created.rbegin(); it != created.rend(); ++it) {
                registry.DestroyEntity(*it);
            }
            LOG_ERROR("EditorCommands", std::string("Could not restore deleted subtree: ") + error.what());
            return;
        }
        MarkSceneChanged(&ctx);
        m_Succeeded = true;
    }

    std::string_view DeleteEntityCommand::Name() const noexcept { return "Delete entity"; }


    //
    // Project
    //
    ProjectConfigUpdateCommand::ProjectConfigUpdateCommand(const Config::ProjectConfig &oldCfg, const Config::ProjectConfig &newCfg) : m_OldData(oldCfg), m_NewData(newCfg) {}

    void ProjectConfigUpdateCommand::Execute(Core::EngineContext &ctx) {
        if (ctx.projectConfig)
            *ctx.projectConfig = m_NewData;
        if (const auto project = Core::Project::GetActive())
            project->MarkAsChanged();
        RefreshWindowTitle(ctx);
    }

    void ProjectConfigUpdateCommand::Undo(Core::EngineContext &ctx) {
        if (ctx.projectConfig)
            *ctx.projectConfig = m_OldData;
        if (const auto project = Core::Project::GetActive())
            project->MarkAsChanged();
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
        snap.flags = element->HasFlag(::UI::VISIBLE) ? snap.flags | ::UI::VISIBLE : snap.flags & ~::UI::VISIBLE;
        snap.flags = element->HasFlag(::UI::ENABLED) ? snap.flags | ::UI::ENABLED : snap.flags & ~::UI::ENABLED;

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
        if (auto el = CreateUIElementFromSnapshot(m_Snapshot); el && m_UISystem && m_Parent) {
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
    RemoveUIElementCommand::RemoveUIElementCommand(::UI::UISystem *sys, ::UI::UIElement *parent, const ::UI::UIElement *child)
        : m_UISystem(sys), m_Parent(parent), m_Snapshot(SnapshotUIElement(child)), m_RemovedChild(const_cast<::UI::UIElement *>(child)) {}

    void RemoveUIElementCommand::Execute(Core::EngineContext &ctx) {
        if (m_Restored && m_UISystem && m_Parent) {
            // Re-remove on redo
            m_UISystem->RemoveChild(m_Parent, m_Restored);
            m_Restored = nullptr;
        } else if (!m_Snapshot.name.empty() && m_UISystem && m_Parent) {
            for (auto *child : m_Parent->Children) {
                if (child == m_RemovedChild) {
                    m_UISystem->RemoveChild(m_Parent, child);
                    return;
                }
            }
            for (auto *child : m_Parent->Children) {
                if (child->Name == m_Snapshot.name && child->Rect.Position == m_Snapshot.position && child->Rect.Scale == m_Snapshot.scale) {
                    m_UISystem->RemoveChild(m_Parent, child);
                    break;
                }
            }
        }
    }

    void RemoveUIElementCommand::Undo(Core::EngineContext &ctx) {
        if (auto el = CreateUIElementFromSnapshot(m_Snapshot); el && m_UISystem && m_Parent) {
            m_Restored = m_UISystem->AddChild(m_Parent, std::move(el));
        }
    }

    //
    // Themeing
    //
    ThemeUpdateCommand::ThemeUpdateCommand(const UI::Theme::Theme &oldCfg, const UI::Theme::Theme &newCfg, const UI::Theme::FontSet &oldSet, const UI::Theme::FontSet &newSet, EditorContext &eCtx)
        : m_EditorCtx(&eCtx), m_OldData(oldCfg), m_NewData(newCfg), m_OldSet(oldSet), m_NewSet(newSet) {}

    void ThemeUpdateCommand::Execute(Core::EngineContext &ctx) {
        m_EditorCtx->theme = m_NewData;
        UI::Theme::Apply(m_NewData);

        m_EditorCtx->fontset = m_NewSet;

        if (m_EditorCtx->fontsDirty) {
            LOG_INFO("ThemeCmd", "Execute: Setting fontsDirty = true");
            m_EditorCtx->fontsDirty->store(true, std::memory_order_release);
        }

        UI::Theme::IO::Serialize(*m_EditorCtx);
    }


    void ThemeUpdateCommand::Undo(Core::EngineContext &ctx) {
        m_EditorCtx->theme = m_OldData;
        UI::Theme::Apply(m_OldData);

        m_EditorCtx->fontset = m_OldSet;
        LOG_INFO("ThemeCmd", "Undo: Setting fontsDirty = true");
        m_EditorCtx->fontsDirty->store(true, std::memory_order_release);

        UI::Theme::IO::Serialize(*m_EditorCtx);
    }
    std::string_view ThemeUpdateCommand::Name() const noexcept { return "Theme change"; }

    //
    // post proc
    //
    PostProcUpdateCommand::PostProcUpdateCommand(const std::vector<Rendering::PostProcessing::PostEffect> &oldFx, const std::vector<Rendering::PostProcessing::PostEffect> &newFx) : m_OldData(oldFx), m_NewData(newFx) {}

    void PostProcUpdateCommand::Execute(Core::EngineContext &ctx) {
        ctx.renderer->GetPostProcessor().Effects() = m_NewData;
        MarkSceneChanged(&ctx);
    }

    void PostProcUpdateCommand::Undo(Core::EngineContext &ctx) {
        ctx.renderer->GetPostProcessor().Effects() = m_OldData;
        MarkSceneChanged(&ctx);
    }

    //
    // Clipboard
    //

    void PasteEntityCommand::Execute(Core::EngineContext &ctx) {
        m_Created = ECS::INVALID_ENTITY_ID;

        Scenes::Scene *scene = ctx.sceneManager ? ctx.sceneManager->GetCurrentScene() : nullptr;
        if (!scene || m_Data.empty())
            return;

        if (m_Data.contains("entities")) {
            m_Created = ECS::Utils::InsertEntityTreeJson(m_Data, &scene->GetRegistry(), m_ParentOverride);
        } else {
            nlohmann::json data = m_Data;
            if (data.contains("name"))
                data["name"] = data["name"].get<std::string>() + " (copy)";
            m_Created = ECS::Utils::InsertEntityJson(std::move(data), &scene->GetRegistry());
            if (m_Created != ECS::INVALID_ENTITY_ID && m_ParentOverride != ECS::INVALID_ENTITY_ID && scene->GetRegistry().IsValid(m_ParentOverride))
                scene->GetRegistry().Reparent(m_Created, m_ParentOverride);
        }

        if (m_Created != ECS::INVALID_ENTITY_ID)
            MarkSceneChanged(&ctx);
    }

    void PasteEntityCommand::Undo(Core::EngineContext &ctx) {
        Scenes::Scene *scene = ctx.sceneManager ? ctx.sceneManager->GetCurrentScene() : nullptr;
        if (!scene || m_Created == ECS::INVALID_ENTITY_ID || !scene->GetRegistry().IsValid(m_Created))
            return;

        scene->GetRegistry().DestroyEntity(m_Created);
        m_Created = ECS::INVALID_ENTITY_ID;
        MarkSceneChanged(&ctx);
    }

    void PasteUIElementCommand::Execute(Core::EngineContext &ctx) {
        m_Created = nullptr;

        ::UI::UISystem *ui = ctx.uiSystem;
        if (!ui || m_Data.empty())
            return;

        nlohmann::json data = m_Data;

        const std::string base = data.value("name", "Element");
        std::string name = base;
        for (int suffix = 1; ui->FindByName(name); ++suffix)
            name = base + " (" + std::to_string(suffix) + ")";
        data["name"] = name;

        ::UI::UIElement *parent = m_Parent ? m_Parent : ui->GetRoot();
        try {
            m_Created = IO::UISerializer::DeserializeElementTree(data, parent, *ui, *ctx.resources);
        } catch (const std::exception &) {
            m_Created = nullptr;
        }

        if (m_Created)
            MarkSceneChanged(&ctx);
    }

    void PasteUIElementCommand::Undo(Core::EngineContext &ctx) {
        if (!ctx.uiSystem || !m_Created)
            return;

        if (::UI::UIElement *parent = m_Created->Parent)
            ctx.uiSystem->RemoveChild(parent, m_Created);
        m_Created = nullptr;
        MarkSceneChanged(&ctx);
    }

} // namespace Editor::Commands
