#pragma once

#include "ComponentPool.h"
#include "Entity.h"
#include "Types.h"
#include <cassert>
#include <memory>
#include <queue>
#include <ranges>
#include <typeindex>
#include <unordered_map>

namespace ECS {
    class Registry {
    private:
        std::queue<uint32_t> m_AvailableEntities;
        std::unordered_map<std::type_index, std::unique_ptr<IPool>> m_ComponentPools;
        std::vector<EntityID> m_LivingEntities;
        std::unordered_map<EntityID, std::string> m_EntityNames;
        std::vector<bool> m_EntityStatus;
        std::vector<uint32_t> m_EntityVersions;

        template <typename T> ComponentPool<T> *GetPool() {
            const auto type = std::type_index(typeid(T));
            auto [it, inserted] = m_ComponentPools.try_emplace(type);
            if (inserted) {
                it->second = std::make_unique<ComponentPool<T>>();
            }
            return static_cast<ComponentPool<T> *>(it->second.get());
        }

    public:
        Registry() {
            m_EntityStatus.resize(MAX_ENTITIES, false);
            m_EntityVersions.resize(MAX_ENTITIES, 0);
            for (uint32_t i = 0; i < MAX_ENTITIES; ++i) {
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

            const uint32_t index = GetEntityIndex(id);

            for (const auto &pool : m_ComponentPools | std::views::values) {
                pool->EntityDestroyed(id);
            }

            // Increment generation to invalidate old handles
            m_EntityVersions[index]++;
            m_EntityStatus[index] = false;
            m_AvailableEntities.push(index);

            std::erase(m_LivingEntities, id);
        }

        bool IsValid(const EntityID id) const {
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

        void SetEntityName(const EntityID id, const std::string &name) { m_EntityNames[id] = name; }

        const std::string &GetEntityName(const EntityID id) const {
            if (const auto it = m_EntityNames.find(id); it != m_EntityNames.end()) {
                return it->second;
            }
            static std::string empty;
            return empty;
        }

        const std::vector<EntityID> &GetLivingEntities() const { return m_LivingEntities; }

        template <typename Primary, typename... Rest, typename Func> void ForEach(Func &&func) {
            auto *primaryPool = GetPool<Primary>();
            auto restPools = std::make_tuple(GetPool<Rest>()...);

            for (EntityID id : primaryPool->GetDenseEntities()) {
                if ((std::get<ComponentPool<Rest> *>(restPools)->Has(id) && ...)) {
                    func(Entity(id, this), primaryPool->Get(id),
                         std::get<ComponentPool<Rest> *>(restPools)->Get(id)...);
                }
            }
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
} // namespace ECS
