#include "EditorCommands.h"
#include "Applications/Editor/Commands/CustomDataSnapshot.h"
#include "Core/Project.h"
#include "ECS/Entity.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MapComponent.h"
#include "Applications/Editor/UI/Panels/Editor/EditorWidgets.h"
#include "Applications/Editor/UI/Themeing/ThemeSerializer.h"
#include "ObSL/ScriptRuntime.h"
#include "Rendering/Renderer.h"
#include "Scenes/SceneManager.h"
#include "Core/Utils/ECSUtils.h"
#include "IO/Loaders/UISerializer.h"
#include "UI/Elements/UIRect.h"
#include "UI/Elements/UIText.h"
#include "UI/Elements/UIImage.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/DirectionalTextureComponent.h"
#include "ECS/Components/BillboardTagComponent.h"
#include "ECS/Components/PointLightComponent.h"
#include "ECS/Components/ColliderComponent.h"
#include "ECS/Components/SpriteSheetComponent.h"
#include "ECS/Components/SpriteAnimatorComponent.h"
#include "ECS/Components/ParticleEmitterComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Components/PersistentTagComponent.h"
#include "ECS/Components/PrefabSourceComponent.h"
#include "ECS/Components/CustomDataComponent.h"
#include "ECS/Components/DestroyTagComponent.h"
#include "IO/Loaders/SceneAssetLoader.h"
#include "nlohmann/json_fwd.hpp"
#include <algorithm>
#include <limits>
#include <map>
#include <string>
#include <tuple>
#include <unordered_set>
#include <stdexcept>

namespace Editor::Commands {

    namespace {
        ECS::EntityID FindUUID(const ECS::Registry &registry, const Core::UUID &uuid) {
            if (uuid.IsNil()) {
                return ECS::INVALID_ENTITY_ID;
            }
            return registry.FindEntityByUUID(uuid);
        }

        std::optional<std::size_t> ResolveScriptSlot(const ECS::Components::ScriptComponent &scripts, const std::size_t slotIndex, const std::string &scriptPath) {
            if (slotIndex < scripts.slots.size() && scripts.slots[slotIndex].scriptPath == scriptPath) {
                return slotIndex;
            }
            for (std::size_t i = 0; i < scripts.slots.size(); ++i) {
                if (scripts.slots[i].scriptPath == scriptPath) {
                    return i;
                }
            }
            return std::nullopt;
        }

        struct ScriptSlotSnapshot {
            std::string path;
            std::filesystem::path resolvedPath;
            std::string source;
            std::filesystem::file_time_type lastModified;
            std::map<std::string, nlohmann::json> properties;
        };

        // Copy editor state and resource ownership, without retaining script runtime pointers.
        using ComponentSnapshots = std::tuple<std::optional<ECS::Components::TransformComponent>, std::optional<ECS::Components::MovementComponent>, std::optional<ECS::Components::MeshComponent>,
                std::optional<ECS::Components::MaterialComponent>, std::optional<ECS::Components::DirectionalTextureComponent>, std::optional<ECS::Components::BillboardTagComponent>,
                std::optional<ECS::Components::PointLightComponent>, std::optional<ECS::Components::ColliderComponent>, std::optional<ECS::Components::SpriteSheetComponent>,
                std::optional<ECS::Components::SpriteAnimatorComponent>, std::optional<ECS::Components::ParticleEmitterComponent>, std::optional<ECS::Components::MapComponent>,
                std::optional<ECS::Components::MapStateComponent>, std::optional<ECS::Components::PersistentTagComponent>, std::optional<ECS::Components::PrefabSourceComponent>>;

        template <typename T> void CaptureComponent(ECS::Registry &registry, const ECS::EntityID id, std::optional<T> &saved) {
            if (const auto *component = registry.GetComponent<T>(id)) {
                saved = *component;
            }
        }

        template <typename T> void RestoreComponent(ECS::Registry &registry, const ECS::EntityID id, const std::optional<T> &saved) {
            if (saved) {
                registry.AddComponent<T>(id, *saved);
            }
        }

        struct SavedEntity {
            Core::UUID uuid;
            std::string name;
            Core::UUID parentUUID;
            std::size_t siblingIndex = 0;
            bool hadRelationship = false;
            std::optional<CustomDataSnapshot> customData;
            ComponentSnapshots components;
            std::optional<std::vector<ScriptSlotSnapshot>> scripts;
            nlohmann::json assetReferences;
        };
    } // namespace

    struct EntitySubtreeSnapshot {
        std::vector<SavedEntity> entities;
    };

    namespace {
        std::unique_ptr<EntitySubtreeSnapshot> CaptureSubtree(ECS::Registry &registry, const ECS::EntityID root, Core::ResourceManager &resources) {
            IO::EntityFactory::RegisterSerializers();
            auto snapshot = std::make_unique<EntitySubtreeSnapshot>();
            std::unordered_set<ECS::EntityID> visited;
            const auto capture = [&](auto &self, const ECS::EntityID id) -> void {
                if (!registry.IsValid(id) || !visited.insert(id).second) {
                    throw std::runtime_error("Invalid or cyclic entity hierarchy");
                }
                if (registry.HasComponent<ECS::Components::DestroyTagComponent>(id)) {
                    throw std::runtime_error("Entity is already scheduled for destruction");
                }
                const auto *custom = registry.GetComponent<ECS::Components::CustomDataComponent>(id);

                SavedEntity saved;
                saved.uuid = registry.GetEntityUUID(id);
                saved.name = registry.GetEntityName(id);
                if (custom) {
                    saved.customData = CaptureCustomDataSnapshot(custom->script_components);
                }
                std::apply([&](auto &...component) { (CaptureComponent(registry, id, component), ...); }, saved.components);
                if (const auto *scripts = registry.GetComponent<ECS::Components::ScriptComponent>(id)) {
                    saved.scripts.emplace();
                    for (const auto &slot : scripts->slots) {
                        saved.scripts->push_back({.path = slot.scriptPath, .resolvedPath = slot.resolvedPath, .source = slot.source_code, .lastModified = slot.lastModified, .properties = slot.properties});
                    }
                }

                std::vector<ECS::EntityID> children;
                if (const auto *relationship = registry.GetComponent<ECS::Components::RelationshipComponent>(id)) {
                    saved.hadRelationship = true;
                    children = relationship->children;
                    if (relationship->parent != ECS::INVALID_ENTITY_ID) {
                        saved.parentUUID = registry.GetEntityUUID(relationship->parent);
                        const auto *parent = registry.GetComponent<ECS::Components::RelationshipComponent>(relationship->parent);
                        if (saved.parentUUID.IsNil() || !parent) {
                            throw std::runtime_error("Entity has an invalid parent");
                        }
                        const auto it = std::ranges::find(parent->children, id);
                        if (it == parent->children.end()) {
                            throw std::runtime_error("Entity is missing from its parent's children");
                        }
                        saved.siblingIndex = static_cast<std::size_t>(std::distance(parent->children.begin(), it));
                    }
                }

                ECS::Entity entity(id, &registry);
                saved.assetReferences = {{"components", nlohmann::json::object()}};
                IO::EntityFactory::SerializeEntity(entity, saved.assetReferences, resources);
                snapshot->entities.push_back(std::move(saved));
                for (const auto child : children) {
                    self(self, child);
                }
            };
            capture(capture, root);
            return snapshot;
        }

        ECS::EntityID RestoreSubtree(const EntitySubtreeSnapshot &snapshot, Scenes::Scene &scene) {
            auto &registry = scene.GetRegistry();
            if (snapshot.entities.empty()) {
                throw std::runtime_error("Empty entity snapshot");
            }
            if (snapshot.entities.size() > ECS::MAX_ENTITIES - 1 - registry.GetLivingEntities().size()) {
                throw std::runtime_error("Not enough free entity slots to restore subtree");
            }
            std::unordered_set<Core::UUID> uuids;
            nlohmann::json references = {{"entities", nlohmann::json::array()}};
            for (const auto &saved : snapshot.entities) {
                if (saved.uuid.IsNil() || !uuids.insert(saved.uuid).second || FindUUID(registry, saved.uuid) != ECS::INVALID_ENTITY_ID) {
                    throw std::runtime_error("Restored entity UUID already exists");
                }
                references["entities"].push_back(saved.assetReferences);
            }
            for (const auto &saved : snapshot.entities) {
                if (!saved.parentUUID.IsNil() && !uuids.contains(saved.parentUUID) && FindUUID(registry, saved.parentUUID) == ECS::INVALID_ENTITY_ID) {
                    throw std::runtime_error("Restored entity's parent no longer exists");
                }
            }

            IO::SceneAssetLoader::SceneAssetScope assets;
            if (!IO::SceneAssetLoader::LoadReferenced(references, assets)) {
                throw std::runtime_error("Could not load restored entity assets");
            }

            std::vector<ECS::EntityID> created;
            created.reserve(snapshot.entities.size());
            try {
                for (const auto &saved : snapshot.entities) {
                    const auto id = registry.CreateEntity();
                    if (id == ECS::INVALID_ENTITY_ID) {
                        throw std::runtime_error("Could not create restored entity");
                    }
                    created.push_back(id);
                    if (!registry.SetEntityUUID(id, saved.uuid)) {
                        throw std::runtime_error("Could not restore entity UUID");
                    }
                    registry.SetEntityName(id, saved.name);
                    std::apply([&](const auto &...component) { (RestoreComponent(registry, id, component), ...); }, saved.components);
                    if (saved.hadRelationship) {
                        registry.AddComponent<ECS::Components::RelationshipComponent>(id);
                    }

                    if (saved.customData) {
                        auto &custom = registry.AddComponent<ECS::Components::CustomDataComponent>(id);

                        if (!saved.customData->components.empty()) {
                            auto *runtime = scene.GetContext().scriptPool;
                            if (!runtime || runtime->worker_count() == 0) {
                                throw std::runtime_error("Cannot restore custom data without a script interpreter");
                            }

                            auto &interpreter = runtime->get_worker(0)->GetInterpreter();

                            ObSL::GCProtectScope const protection(&interpreter);

                            const auto values = RestoreCustomDataSnapshot(*saved.customData, interpreter, protection);

                            for (const auto &[name, value] : values) {
                                custom.Set(name, value, interpreter);
                            }
                        }
                    }

                    if (saved.scripts) {
                        auto &scripts = registry.AddComponent<ECS::Components::ScriptComponent>(id);
                        for (const auto &slot : *saved.scripts) {
                            ECS::Components::ScriptSlot restored;
                            restored.scriptPath = slot.path;
                            restored.resolvedPath = slot.resolvedPath;
                            restored.source_code = slot.source;
                            restored.lastModified = slot.lastModified;
                            restored.properties = slot.properties;
                            scripts.slots.push_back(std::move(restored));
                        }
                    }
                    if (auto *map = registry.GetComponent<ECS::Components::MapComponent>(id)) {
                        map->needsMeshUpdate = true;
                        map->lightmap.lastLightCount = std::numeric_limits<std::size_t>::max();
                    }
                    if (auto *particles = registry.GetComponent<ECS::Components::ParticleEmitterComponent>(id)) {
                        particles->emitterIndex = -1;
                        particles->aliveCount = 0;
                        particles->isDirty = true;
                    }
                }
                for (std::size_t i = 0; i < snapshot.entities.size(); ++i) {
                    const auto &saved = snapshot.entities[i];
                    if (saved.parentUUID.IsNil()) {
                        continue;
                    }
                    const auto parent = FindUUID(registry, saved.parentUUID);
                    registry.SetParentDirect(created[i], parent);
                    auto &siblings = registry.GetComponent<ECS::Components::RelationshipComponent>(parent)->children;
                    std::erase(siblings, created[i]);
                    siblings.insert(siblings.begin() + static_cast<std::ptrdiff_t>(std::min(saved.siblingIndex, siblings.size())), created[i]);
                }
                scene.AddAssetScope(std::move(assets));
            } catch (...) {
                for (auto it = created.rbegin(); it != created.rend(); ++it) {
                    registry.DestroyEntity(*it);
                }
                throw;
            }
            return created.front();
        }
    } // namespace

    ECS::Entity EntityCommand::ResolveEntity(const Core::EngineContext &ctx) {
        m_Succeeded = false;
        auto *scene = ctx.sceneManager ? ctx.sceneManager->GetCurrentScene() : nullptr;
        if (!scene) {
            return {};
        }
        auto &registry = scene->GetRegistry();
        const auto id = FindUUID(registry, m_EntityUUID);
        return id != ECS::INVALID_ENTITY_ID ? ECS::Entity(id, &registry) : ECS::Entity{};
    }

    void EntityCommand::Complete(const Core::EngineContext &ctx) {
        MarkSceneChanged(&ctx);
        m_Succeeded = true;
    }

    //
    // Transforms
    //

    // Move Transform
    TranslateEntityCommand::TranslateEntityCommand(Core::UUID targetUUID, const glm::vec3 oldPos, const glm::vec3 newPos) : EntityCommand(targetUUID), m_OldPos(oldPos), m_NewPos(newPos) {}

    void TranslateEntityCommand::Execute(Core::EngineContext &ctx) {
        const auto entity = ResolveEntity(ctx);
        if (!entity) {
            return;
        }
        if (auto *transform = entity.GetComponent<ECS::Components::TransformComponent>()) {
            transform->transform.SetPosition(m_NewPos);
            Complete(ctx);
        }
    }

    void TranslateEntityCommand::Undo(Core::EngineContext &ctx) {
        const auto entity = ResolveEntity(ctx);
        if (!entity) {
            return;
        }
        if (auto *transform = entity.GetComponent<ECS::Components::TransformComponent>()) {
            transform->transform.SetPosition(m_OldPos);
            Complete(ctx);
        }
    }

    std::string_view TranslateEntityCommand::Name() const noexcept { return "Move entity"; }

    // Rotate Transform
    RotateEntityCommand::RotateEntityCommand(Core::UUID targetUUID, const glm::vec3 oldRot, const glm::vec3 newRot) : EntityCommand(targetUUID), m_OldRot(oldRot), m_NewRot(newRot) {}

    void RotateEntityCommand::Execute(Core::EngineContext &ctx) {
        const auto entity = ResolveEntity(ctx);
        if (!entity) {
            return;
        }
        if (auto *transform = entity.GetComponent<ECS::Components::TransformComponent>()) {
            transform->transform.SetRotation(m_NewRot);
            Complete(ctx);
        }
    }

    void RotateEntityCommand::Undo(Core::EngineContext &ctx) {
        const auto entity = ResolveEntity(ctx);
        if (!entity) {
            return;
        }
        if (auto *transform = entity.GetComponent<ECS::Components::TransformComponent>()) {
            transform->transform.SetRotation(m_OldRot);
            Complete(ctx);
        }
    }

    std::string_view RotateEntityCommand::Name() const noexcept { return "Rotate entity"; }

    // Scale Transform
    ScaleEntityCommand::ScaleEntityCommand(Core::UUID targetUUID, const glm::vec3 oldScale, const glm::vec3 newScale) : EntityCommand(targetUUID), m_OldScale(oldScale), m_NewScale(newScale) {}

    void ScaleEntityCommand::Execute(Core::EngineContext &ctx) {
        const auto entity = ResolveEntity(ctx);
        if (!entity) {
            return;
        }
        if (auto *transform = entity.GetComponent<ECS::Components::TransformComponent>()) {
            transform->transform.SetScale(m_NewScale);
            Complete(ctx);
        }
    }

    void ScaleEntityCommand::Undo(Core::EngineContext &ctx) {
        const auto entity = ResolveEntity(ctx);
        if (!entity) {
            return;
        }
        if (auto *transform = entity.GetComponent<ECS::Components::TransformComponent>()) {
            transform->transform.SetScale(m_OldScale);
            Complete(ctx);
        }
    }

    std::string_view ScaleEntityCommand::Name() const noexcept { return "Scale entity"; }

    TransformEntityCommand::TransformEntityCommand(Core::UUID targetUUID, const Rendering::Transform &before, const Rendering::Transform &after) : EntityCommand(targetUUID), m_Before(before), m_After(after) {}

    void TransformEntityCommand::Apply(Core::EngineContext &ctx, const Rendering::Transform &pose) {
        const auto entity = ResolveEntity(ctx);
        if (!entity)
            return;
        if (auto *transform = entity.GetComponent<ECS::Components::TransformComponent>()) {
            transform->transform = pose;
            Complete(ctx);
        }
    }

    void TransformEntityCommand::Execute(Core::EngineContext &ctx) { Apply(ctx, m_After); }
    void TransformEntityCommand::Undo(Core::EngineContext &ctx) { Apply(ctx, m_Before); }

    //
    // Rename Entity
    //

    SetNameCommand::SetNameCommand(Core::UUID targetUUID, std::string oldName, std::string newName) : EntityCommand(targetUUID), m_OldName(std::move(oldName)), m_NewName(std::move(newName)) {}

    void SetNameCommand::Execute(Core::EngineContext &ctx) {
        const auto entity = ResolveEntity(ctx);
        if (entity) {
            entity.SetName(m_NewName);
            Complete(ctx);
        }
    }

    void SetNameCommand::Undo(Core::EngineContext &ctx) {
        const auto entity = ResolveEntity(ctx);
        if (entity) {
            entity.SetName(m_OldName);
            Complete(ctx);
        }
    }

    std::string_view SetNameCommand::Name() const noexcept { return "Rename entity"; }

    // normal components commands are in the header (EditorCommands.h)

    //
    // Script Component
    //

    // Remove script
    RemoveScriptCommand::RemoveScriptCommand(Core::UUID targetUUID, const int index) : EntityCommand(targetUUID), m_Index(index) {}

    void RemoveScriptCommand::Execute(Core::EngineContext &ctx) {
        const auto entity = ResolveEntity(ctx);
        if (!entity) {
            return;
        }
        auto *scripts = entity.GetComponent<ECS::Components::ScriptComponent>();
        if (!scripts || m_Index < 0 || static_cast<std::size_t>(m_Index) >= scripts->slots.size()) {
            return;
        }
        const auto &slot = scripts->slots[m_Index];
        // save only copyable fields for the entry being removed
        m_SavedPath = slot.scriptPath;
        m_SavedResolvedPath = slot.resolvedPath;
        m_SavedSourceCode = slot.source_code;
        m_SavedLastModified = slot.lastModified;
        m_SavedProperties = slot.properties;
        scripts->slots.erase(scripts->slots.begin() + m_Index);
        m_ComponentRemoved = scripts->slots.empty();
        if (m_ComponentRemoved) {
            entity.RemoveComponent<ECS::Components::ScriptComponent>();
        }
        Complete(ctx);
    }

    void RemoveScriptCommand::Undo(Core::EngineContext &ctx) {
        auto entity = ResolveEntity(ctx);
        if (!entity) {
            return;
        }
        auto *scripts = entity.GetComponent<ECS::Components::ScriptComponent>();
        if (m_ComponentRemoved) {
            if (scripts || m_Index != 0) {
                return;
            }
            scripts = &entity.AddComponent<ECS::Components::ScriptComponent>();
        } else if (!scripts || m_Index < 0 || static_cast<std::size_t>(m_Index) > scripts->slots.size()) {
            return;
        }
        // insert saved data back at the original index
        ECS::Components::ScriptSlot slot;
        slot.scriptPath = m_SavedPath;
        slot.resolvedPath = m_SavedResolvedPath;
        slot.source_code = m_SavedSourceCode;
        slot.lastModified = m_SavedLastModified;
        slot.properties = m_SavedProperties;
        scripts->slots.insert(scripts->slots.begin() + m_Index, std::move(slot));
        Complete(ctx);
    }

    std::string_view RemoveScriptCommand::Name() const noexcept { return "Remove Script"; }

    // Add script
    AddScriptCommand::AddScriptCommand(Core::UUID targetUUID, const std::string &script_path) : EntityCommand(targetUUID), m_PendingPath(script_path) {}

    void AddScriptCommand::Execute(Core::EngineContext &ctx) {
        auto entity = ResolveEntity(ctx);
        if (!entity || m_PendingPath.empty()) {
            return;
        }
        auto *scripts = entity.GetComponent<ECS::Components::ScriptComponent>();
        if (!m_Captured) {
            m_ComponentCreated = scripts == nullptr;
            m_Index = scripts ? scripts->slots.size() : 0;
        }
        if (!scripts) {
            if (!m_ComponentCreated) {
                return;
            }
            scripts = &entity.AddComponent<ECS::Components::ScriptComponent>();
        }
        if (scripts->slots.size() != m_Index) {
            return;
        }
        ECS::Components::ScriptSlot slot;
        slot.scriptPath = m_PendingPath;
        scripts->slots.push_back(std::move(slot));
        m_Captured = true;
        Complete(ctx);
    }

    void AddScriptCommand::Undo(Core::EngineContext &ctx) {
        const auto entity = ResolveEntity(ctx);
        if (!entity) {
            return;
        }
        auto *scripts = entity.GetComponent<ECS::Components::ScriptComponent>();
        if (!m_Captured || !scripts || m_Index >= scripts->slots.size() || scripts->slots[m_Index].scriptPath != m_PendingPath) {
            return;
        }
        scripts->slots.erase(scripts->slots.begin() + static_cast<std::ptrdiff_t>(m_Index));
        if (m_ComponentCreated && scripts->slots.empty()) {
            entity.RemoveComponent<ECS::Components::ScriptComponent>();
        }
        Complete(ctx);
    }

    std::string_view AddScriptCommand::Name() const noexcept { return "Add script"; }

    SetScriptPropertyCommand::SetScriptPropertyCommand(
            Core::UUID targetUUID, const std::size_t slotIndex, std::string scriptPath, std::string name, std::optional<nlohmann::json> oldValue, std::optional<nlohmann::json> newValue)
        : EntityCommand(targetUUID), m_SlotIndex(slotIndex), m_ScriptPath(std::move(scriptPath)), m_Name(std::move(name)), m_Old(std::move(oldValue)), m_New(std::move(newValue)) {}

    std::optional<std::size_t> SetScriptPropertyCommand::ResolveSlot(const ECS::Components::ScriptComponent &scripts) const { return ResolveScriptSlot(scripts, m_SlotIndex, m_ScriptPath); }

    void SetScriptPropertyCommand::Apply(Core::EngineContext &ctx, const std::optional<nlohmann::json> &value) {
        const auto entity = ResolveEntity(ctx);
        if (!entity) {
            return;
        }
        auto *scripts = entity.GetComponent<ECS::Components::ScriptComponent>();
        if (!scripts) {
            return;
        }
        const auto slotIndex = ResolveSlot(*scripts);
        if (!slotIndex) {
            return;
        }
        auto &properties = scripts->slots[*slotIndex].properties;
        if (value) {
            properties[m_Name] = *value;
        } else {
            properties.erase(m_Name);
        }
        Complete(ctx);
    }

    void SetScriptPropertyCommand::Execute(Core::EngineContext &ctx) { Apply(ctx, m_New); }

    void SetScriptPropertyCommand::Undo(Core::EngineContext &ctx) { Apply(ctx, m_Old); }

    std::string_view SetScriptPropertyCommand::Name() const noexcept { return "Change script property"; }

    PruneScriptPropertiesCommand::PruneScriptPropertiesCommand(Core::UUID targetUUID, const std::size_t slotIndex, std::string scriptPath, std::map<std::string, nlohmann::json> staleValues)
        : EntityCommand(targetUUID), m_SlotIndex(slotIndex), m_ScriptPath(std::move(scriptPath)), m_StaleValues(std::move(staleValues)) {}

    std::optional<std::size_t> PruneScriptPropertiesCommand::ResolveSlot(const ECS::Components::ScriptComponent &scripts) const { return ResolveScriptSlot(scripts, m_SlotIndex, m_ScriptPath); }

    void PruneScriptPropertiesCommand::Apply(Core::EngineContext &ctx, const bool remove) {
        const auto entity = ResolveEntity(ctx);
        if (!entity) {
            return;
        }
        auto *scripts = entity.GetComponent<ECS::Components::ScriptComponent>();
        if (!scripts) {
            return;
        }
        const auto slotIndex = ResolveSlot(*scripts);
        if (!slotIndex) {
            return;
        }
        auto &properties = scripts->slots[*slotIndex].properties;
        if (remove) {
            for (const auto &name : m_StaleValues | std::views::keys) {
                properties.erase(name);
            }
        } else {
            for (const auto &[name, value] : m_StaleValues) {
                properties[name] = value;
            }
        }
        Complete(ctx);
    }

    void PruneScriptPropertiesCommand::Execute(Core::EngineContext &ctx) { Apply(ctx, true); }

    void PruneScriptPropertiesCommand::Undo(Core::EngineContext &ctx) { Apply(ctx, false); }

    std::string_view PruneScriptPropertiesCommand::Name() const noexcept { return "Remove stale script properties"; }


    DeleteEntityCommand::DeleteEntityCommand(Core::UUID targetUUID) : EntityCommand(targetUUID) {}
    DeleteEntityCommand::~DeleteEntityCommand() = default;

    void DeleteEntityCommand::Execute(Core::EngineContext &ctx) {
        const auto entity = ResolveEntity(ctx);
        if (!entity || !ctx.resources) {
            return;
        }
        auto &registry = *entity.GetRegistry();
        if (!m_Snapshot) {
            m_Snapshot = CaptureSubtree(registry, static_cast<ECS::EntityID>(entity), *ctx.resources);
        }
        registry.DestroyEntity(static_cast<ECS::EntityID>(entity));
        Complete(ctx);
    }

    void DeleteEntityCommand::Undo(Core::EngineContext &ctx) {
        m_Succeeded = false;
        auto *scene = ctx.sceneManager ? ctx.sceneManager->GetCurrentScene() : nullptr;
        if (!scene || !m_Snapshot) {
            return;
        }
        RestoreSubtree(*m_Snapshot, *scene);
        Complete(ctx);
    }

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
    MapChangeTileCommand::MapChangeTileCommand(StateMap oldState, StateMap newState, Core::UUID targetUUID) : EntityCommand(targetUUID), m_OldState(std::move(oldState)), m_NewState(std::move(newState)) {}

    void MapChangeTileCommand::ApplyStates(const Core::EngineContext &ctx, const StateMap &states) {
        const auto entity = ResolveEntity(ctx);
        if (!entity) {
            return;
        }
        auto *map = entity.GetComponent<ECS::Components::MapComponent>();
        if (!map) {
            return;
        }
        for (const auto &[pos, state] : states) {
            map->grid.RemoveTileAt(pos);
            if (state) {
                map->grid.EmplaceTile(pos, state->first, state->second);
            }
        }
        map->needsMeshUpdate = true;
        map->mapDirty = true;
        Complete(ctx);
    }

    void MapChangeTileCommand::Execute(Core::EngineContext &ctx) { ApplyStates(ctx, m_NewState); }

    void MapChangeTileCommand::Undo(Core::EngineContext &ctx) { ApplyStates(ctx, m_OldState); }

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

        snap.uuid = element->GetUUID();
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

        snap.children.reserve(element->Children.size());
        for (const auto *child : element->Children) {
            snap.children.push_back(SnapshotUIElement(child));
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


    ::UI::UIElement *UIElementCommand::ResolveElement(const Core::EngineContext &ctx) {
        m_Succeeded = false;
        return ctx.uiSystem ? ctx.uiSystem->FindByUUID(m_ElementUUID) : nullptr;
    }

    void UIElementCommand::Complete(const Core::EngineContext &ctx) {
        MarkSceneChanged(&ctx);
        m_Succeeded = true;
    }


    // AddUIElementCommand
    AddUIElementCommand::AddUIElementCommand(const ::UI::UISystem *sys, const ::UI::UIElement *parent, UIElementSnapshot snapshot)
        : UIElementCommand(snapshot.uuid.IsNil() ? Core::UUIDGenerator::Generate() : snapshot.uuid), m_ParentUUID(sys && sys->Contains(parent) ? parent->GetUUID() : Core::UUID{}), m_Snapshot(std::move(snapshot)) {
        m_Snapshot.uuid = m_ElementUUID;
    }

    void AddUIElementCommand::Execute(Core::EngineContext &ctx) {
        m_Succeeded = false;

        auto *ui = ctx.uiSystem;
        auto *parent = ui ? ui->FindByUUID(m_ParentUUID) : nullptr;
        if (!parent || ui->FindByUUID(m_ElementUUID)) {
            return;
        }

        auto element = CreateUIElementFromSnapshot(m_Snapshot);
        if (!element || !ui->AddChild(parent, std::move(element), m_ElementUUID)) {
            return;
        }

        Complete(ctx);
    }

    void AddUIElementCommand::Undo(Core::EngineContext &ctx) {
        auto *element = ResolveElement(ctx);
        if (!element || !ctx.uiSystem->Contains(element->Parent)) {
            return;
        }

        ctx.uiSystem->RemoveChild(element->Parent, element);
        Complete(ctx);
    }

    // RemoveUIElementCommand
    RemoveUIElementCommand::RemoveUIElementCommand(const ::UI::UISystem *sys, const ::UI::UIElement *parent, const ::UI::UIElement *child)
        : m_ParentUUID(sys && parent && sys->Contains(parent) ? parent->GetUUID() : Core::UUID{}), m_Snapshot(SnapshotUIElement(child)) {
        if (parent && child) {
            const auto &siblings = parent->Children;
            const auto it = std::ranges::find(siblings, child);
            if (it != siblings.end()) {
                m_SiblingIndex = static_cast<size_t>(std::distance(siblings.begin(), it));
            }
        }
    }

    void RemoveUIElementCommand::Execute(Core::EngineContext &ctx) {
        auto *ui = ctx.uiSystem;
        auto *parent = ui ? ui->FindByUUID(m_ParentUUID) : nullptr;
        if (!parent) {
            return;
        }

        auto *child = ui->FindByUUID(m_Snapshot.uuid);
        if (!child || child->Parent != parent) {
            return;
        }

        ui->RemoveChild(parent, child);
    }

    void RemoveUIElementCommand::RestoreSubtree(::UI::UISystem *ui, ::UI::UIElement *parent, const UIElementSnapshot &snap, const size_t insertIndex) {
        auto element = CreateUIElementFromSnapshot(snap);
        if (!element) {
            return;
        }

        auto *added = ui->AddChild(parent, std::move(element), snap.uuid, insertIndex);
        if (!added) {
            return;
        }

        for (const auto &childSnap : snap.children) {
            RestoreSubtree(ui, added, childSnap, std::numeric_limits<size_t>::max());
        }
    }

    void RemoveUIElementCommand::Undo(Core::EngineContext &ctx) {
        auto *ui = ctx.uiSystem;
        auto *parent = ui ? ui->FindByUUID(m_ParentUUID) : nullptr;
        if (!parent || ui->FindByUUID(m_Snapshot.uuid)) {
            return;
        }

        RestoreSubtree(ui, parent, m_Snapshot, m_SiblingIndex);
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

    PasteEntityCommand::PasteEntityCommand(nlohmann::json data, Core::UUID parentUUID) : EntityCommand(Core::UUIDGenerator::Generate()), m_Data(std::move(data)), m_ParentUUID(parentUUID) {}
    PasteEntityCommand::~PasteEntityCommand() = default;

    void PasteEntityCommand::Execute(Core::EngineContext &ctx) {
        m_Succeeded = false;
        m_Created = ECS::INVALID_ENTITY_ID;
        auto *scene = ctx.sceneManager ? ctx.sceneManager->GetCurrentScene() : nullptr;
        if (!scene || !ctx.resources || m_Data.empty()) {
            return;
        }
        if (m_Snapshot) {
            m_Created = RestoreSubtree(*m_Snapshot, *scene);
            Complete(ctx);
            return;
        }

        auto &registry = scene->GetRegistry();
        const auto parent = FindUUID(registry, m_ParentUUID);
        if (!m_ParentUUID.IsNil() && parent == ECS::INVALID_ENTITY_ID) {
            return;
        }
        const bool tree = m_Data.contains("entities");
        const nlohmann::json entities = tree ? m_Data.at("entities") : nlohmann::json::array({m_Data});
        if (!entities.is_array() || entities.empty()) {
            return;
        }
        const auto rootIndex = tree ? m_Data.value("rootIndex", std::size_t{0}) : 0;
        if (rootIndex >= entities.size() || entities.size() > ECS::MAX_ENTITIES - 1 - registry.GetLivingEntities().size()) {
            return;
        }
        // Check that every entity belongs to the root, and that no parent chain cycles.
        for (std::size_t i = 0; i < entities.size(); ++i) {
            if (!entities[i].is_object()) {
                return;
            }
            std::unordered_set<std::size_t> visited;
            auto current = i;
            while (current != rootIndex) {
                if (!visited.insert(current).second || !entities[current].contains("parent")) {
                    return;
                }
                current = entities[current].at("parent").get<std::size_t>();
                if (current >= entities.size()) {
                    return;
                }
            }
        }

        IO::SceneAssetLoader::SceneAssetScope assets;
        const nlohmann::json references = {{"entities", entities}};
        if (!IO::SceneAssetLoader::LoadReferenced(references, assets)) {
            return;
        }
        IO::EntityFactory::RegisterDeserializers();
        std::vector<ECS::EntityID> created;
        created.reserve(entities.size());
        try {
            for (std::size_t i = 0; i < entities.size(); ++i) {
                const auto id = registry.CreateEntity();
                if (id == ECS::INVALID_ENTITY_ID) {
                    throw std::runtime_error("Could not create pasted entity");
                }
                created.push_back(id);
                auto data = entities[i];
                if (!data.contains("components")) {
                    data["components"] = nlohmann::json::object();
                }
                if (!tree && data.contains("name")) {
                    data["name"] = data["name"].get<std::string>() + " (copy)";
                }
                ECS::Entity entity(id, &registry);
                IO::EntityFactory::DeserializeEntity(entity, data, *ctx.resources);
            }
            if (!registry.SetEntityUUID(created[rootIndex], m_EntityUUID)) {
                throw std::runtime_error("Could not assign pasted entity UUID");
            }
            for (std::size_t i = 0; i < entities.size(); ++i) {
                if (i != rootIndex) {
                    registry.SetParentDirect(created[i], created[entities[i].at("parent").get<std::size_t>()]);
                }
            }
            if (parent != ECS::INVALID_ENTITY_ID) {
                registry.SetParentDirect(created[rootIndex], parent);
            }
            auto snapshot = CaptureSubtree(registry, created[rootIndex], *ctx.resources);
            scene->AddAssetScope(std::move(assets));
            m_Snapshot = std::move(snapshot);
            m_Created = created[rootIndex];
        } catch (...) {
            for (auto it = created.rbegin(); it != created.rend(); ++it) {
                registry.DestroyEntity(*it);
            }
            throw;
        }
        Complete(ctx);
    }

    void PasteEntityCommand::Undo(Core::EngineContext &ctx) {
        const auto entity = ResolveEntity(ctx);
        if (!entity) {
            return;
        }
        entity.GetRegistry()->DestroyEntity(static_cast<ECS::EntityID>(entity));
        m_Created = ECS::INVALID_ENTITY_ID;
        Complete(ctx);
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
