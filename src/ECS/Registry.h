#pragma once
#include "ComponentPool.h"
#include "Entity.h"
#include "Types.h"
#include "ECS/Components/RelationshipComponent.h"
#include "ECS/Components/TransformComponent.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <array>
#include <cassert>
#include <memory>
#include <queue>
#include <ranges>
#include <string>
#include <utility>

namespace ECS {
    inline uint32_t NextPoolIndex() {
        static uint32_t counter = 0;
        return counter++;
    }

    template <typename T> struct ComponentTypeID {
        static uint32_t ID() {
            static uint32_t id = NextPoolIndex();
            return id;
        }
    };

    constexpr uint32_t MAX_COMPONENT_TYPES = 64;

    class Registry {
        std::queue<uint32_t> m_AvailableEntities;
        std::array<std::unique_ptr<IPool>, MAX_COMPONENT_TYPES> m_ComponentPools{};
        std::vector<EntityID> m_LivingEntities;
        std::vector<std::string> m_EntityNames;
        std::vector<bool> m_EntityStatus;
        std::vector<uint32_t> m_EntityVersions;
        // raw cache , mirrors m_ComponentPools but avoids unique_ptr dereferning
        std::array<IPool *, MAX_COMPONENT_TYPES> m_PoolCache{};

    public:
        template <typename T> ComponentPool<T> *GetPool() {
            const uint32_t index = ComponentTypeID<T>::ID();
            assert(index < MAX_COMPONENT_TYPES && "Too many component types!");
            if (!m_ComponentPools[index]) {
                m_ComponentPools[index] = std::make_unique<ComponentPool<T>>();
                m_PoolCache[index] = m_ComponentPools[index].get();
            }
            return static_cast<ComponentPool<T> *>(m_PoolCache[index]);
        }

        Registry() {
            m_EntityStatus.resize(MAX_ENTITIES, false);
            m_EntityVersions.resize(MAX_ENTITIES, 0);
            m_EntityNames.resize(MAX_ENTITIES);
            // 0 is reserved as the invalid/null entity; real entities start at 1
            for (uint32_t i = 1; i < MAX_ENTITIES; ++i) {
                m_AvailableEntities.push(i);
            }
        }

        EntityID CreateEntity() {
            const uint32_t index = m_AvailableEntities.front();
            m_AvailableEntities.pop();

            const uint32_t version = m_EntityVersions[index];
            const EntityID newId = index | version << ENTITY_VERSION_SHIFT;

            m_EntityStatus[index] = true;
            m_LivingEntities.push_back(newId);
            return newId;
        }

        void DestroyEntity(const EntityID id) {
            if (!IsValid(id))
                return;

            // destroy all children first
            if (const auto *rel = GetComponent<Components::RelationshipComponent>(id)) {
                for (const std::vector<EntityID> childrenCopy = rel->children; const EntityID childId : childrenCopy) {
                    DestroyEntity(childId);
                }
            }

            // remove self from parent children list
            if (const auto *rel = GetComponent<Components::RelationshipComponent>(id)) {
                if (rel->parent != INVALID_ENTITY_ID && IsValid(rel->parent)) {
                    if (auto *parentRel = GetComponent<Components::RelationshipComponent>(rel->parent)) {
                        std::erase(parentRel->children, id);
                    }
                }
            }

            const uint32_t index = GetEntityIndex(id);

            for (uint32_t i = 0; i < MAX_COMPONENT_TYPES; ++i) {
                if (m_ComponentPools[i])
                    m_ComponentPools[i]->EntityDestroyed(id);
            }

            // Increment generation to invalidate old handles
            m_EntityVersions[index]++;
            m_EntityStatus[index] = false;
            m_AvailableEntities.push(index);

            if (const auto it = std::ranges::find(m_LivingEntities, id); it != m_LivingEntities.end()) {
                *it = m_LivingEntities.back();
                m_LivingEntities.pop_back();
            }
        }

        [[nodiscard]] bool IsValid(const EntityID id) const {
            const uint32_t index = GetEntityIndex(id);
            if (index >= MAX_ENTITIES || !m_EntityStatus[index]) {
                return false;
            }
            return GetEntityVersion(id) == m_EntityVersions[index];
        }

        template <typename T> void RemoveComponent(const EntityID entity) { GetPool<T>()->EntityDestroyed(entity); }

        template <typename T, typename... Args> T &AddComponent(EntityID entity, Args &&...args) {
            assert(IsValid(entity) && "Attempted to add component to an invalid entity");
            return GetPool<T>()->Emplace(entity, std::forward<Args>(args)...);
        }

        template <typename T> T *GetComponent(EntityID entity) {
            if (!IsValid(entity)) {
                return nullptr;
            }
            return GetPool<T>()->Get(entity);
        }

        template <typename T> bool HasComponent(EntityID entity) { return GetPool<T>()->Has(entity); }

        void SetEntityName(const EntityID id, const std::string &name) { m_EntityNames[GetEntityIndex(id)] = name; }

        [[nodiscard]] const std::string &GetEntityName(const EntityID id) const { return m_EntityNames[GetEntityIndex(id)]; }

        [[nodiscard]] const std::vector<EntityID> &GetLivingEntities() const { return m_LivingEntities; }

        [[nodiscard]] EntityID FindEntityByName(const std::string &name) const {
            for (const EntityID id : m_LivingEntities) {
                if (m_EntityNames[GetEntityIndex(id)] == name)
                    return id;
            }
            return INVALID_ENTITY_ID;
        }

        void Reparent(const EntityID child, const EntityID newParent) {
            if (!IsValid(child) || child == newParent)
                return;

            // preserve world transform before changing hierarchy
            auto *childTrans = GetComponent<Components::TransformComponent>(child);
            const glm::mat4 worldBefore = childTrans ? childTrans->worldTransform.GetMatrix() : glm::mat4(1.0f);

            auto *childRel = GetComponent<Components::RelationshipComponent>(child);
            if (!childRel)
                childRel = &AddComponent<Components::RelationshipComponent>(child);

            // remove from old parent
            if (childRel->parent != INVALID_ENTITY_ID && IsValid(childRel->parent)) {
                if (auto *oldParentRel = GetComponent<Components::RelationshipComponent>(childRel->parent)) {
                    std::erase(oldParentRel->children, child);
                }
            }

            // attach to new parent
            if (newParent != INVALID_ENTITY_ID && IsValid(newParent)) {
                childRel->parent = newParent;
                childRel->parentName = GetEntityName(newParent);

                auto *newParentRel = GetComponent<Components::RelationshipComponent>(newParent);
                if (!newParentRel)
                    newParentRel = &AddComponent<Components::RelationshipComponent>(newParent);
                newParentRel->children.push_back(child);

                // convert world transform to local space relative to new parent
                if (childTrans) {
                    auto *parentTrans = GetComponent<Components::TransformComponent>(newParent);
                    const glm::mat4 parentWorld = parentTrans ? parentTrans->worldTransform.GetMatrix() : glm::mat4(1.0f);
                    const glm::mat4 localRelative = glm::inverse(parentWorld) * worldBefore;
                    glm::vec3 pos, scale, skew;
                    glm::quat rot;
                    glm::vec4 perspective;
                    glm::decompose(localRelative, scale, rot, pos, skew, perspective);
                    childTrans->transform.SetPosition(pos);
                    childTrans->transform.SetRotation(glm::eulerAngles(rot));
                    childTrans->transform.SetScale(scale);
                }
            } else {
                childRel->parent = INVALID_ENTITY_ID;
                childRel->parentName.clear();

                // world becomes local
                if (childTrans) {
                    auto pos = glm::vec3(worldBefore[3]);
                    glm::vec3 scale;
                    glm::quat rot;
                    glm::vec3 skew;
                    glm::vec4 perspective;
                    glm::decompose(worldBefore, scale, rot, pos, skew, perspective);
                    childTrans->transform.SetPosition(pos);
                    childTrans->transform.SetRotation(glm::eulerAngles(rot));
                    childTrans->transform.SetScale(scale);
                }
            }
        }

        template <typename Primary, typename... Rest, typename Func> void ForEach(Func &&func) {
            auto *primaryPool = GetPool<Primary>();
            auto restPools = std::make_tuple(GetPool<Rest>()...);

            for (EntityID id : primaryPool->GetDenseEntities()) {
                if ((std::get<ComponentPool<Rest> *>(restPools)->Has(id) && ...)) {
                    func(Entity(id, this), primaryPool->Get(id), std::get<ComponentPool<Rest> *>(restPools)->Get(id)...);
                }
            }
        }

        template <typename Primary, typename... Rest> EntityID FindFirstEntity() {
            auto *primaryPool = GetPool<Primary>();
            auto restPools = std::make_tuple(GetPool<Rest>()...);

            for (EntityID id : primaryPool->GetDenseEntities()) {
                if ((std::get<ComponentPool<Rest> *>(restPools)->Has(id) && ...)) {
                    return id;
                }
            }
            return INVALID_ENTITY_ID;
        }

        template <typename Primary, typename T = Primary> T *GetFirst() {
            auto *primaryPool = GetPool<Primary>();
            if constexpr (std::is_same_v<Primary, T>) {
                if (!primaryPool->GetDenseEntities().empty())
                    return primaryPool->Get(primaryPool->GetDenseEntities().front());
            } else {
                auto *targetPool = GetPool<T>();
                for (const EntityID id : primaryPool->GetDenseEntities()) {
                    if (T *comp = targetPool->Get(id))
                        return comp;
                }
            }
            return nullptr;
        }
    };

    template <typename T, typename... Args> T &Entity::AddComponent(Args &&...args) { return m_Registry->AddComponent<T>(m_EntityHandle, std::forward<Args>(args)...); }

    template <typename T> T *Entity::GetComponent() const { return m_Registry->GetComponent<T>(m_EntityHandle); }

    template <typename T> bool Entity::HasComponent() const { return m_Registry->HasComponent<T>(m_EntityHandle); }

    template <typename T> void Entity::RemoveComponent() const { m_Registry->RemoveComponent<T>(m_EntityHandle); }

    inline void Entity::SetName(const std::string &name) const { m_Registry->SetEntityName(m_EntityHandle, name); }

    inline const std::string &Entity::GetName() const { return m_Registry->GetEntityName(m_EntityHandle); }

    inline void Entity::SetParent(const EntityID parentId) const { m_Registry->Reparent(m_EntityHandle, parentId); }

    inline Entity Entity::GetParent() const {
        if (const auto *rel = m_Registry->GetComponent<Components::RelationshipComponent>(m_EntityHandle); rel && rel->parent != INVALID_ENTITY_ID && m_Registry->IsValid(rel->parent))
            return Entity(rel->parent, m_Registry);
        return Entity{};
    }

    inline const std::vector<EntityID> &Entity::GetChildren() const {
        static constexpr std::vector<EntityID> empty;
        auto *rel = m_Registry->GetComponent<Components::RelationshipComponent>(m_EntityHandle);
        return rel ? rel->children : empty;
    }
} // namespace ECS
