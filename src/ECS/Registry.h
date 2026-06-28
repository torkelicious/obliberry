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

class Registry {
private:
    std::queue<EntityID> m_AvailableEntities;
    std::unordered_map<std::type_index, std::unique_ptr<IPool> > m_ComponentPools;
    std::vector<EntityID> m_LivingEntities;
    std::unordered_map<EntityID, std::string> m_EntityNames;
    std::vector<bool> m_EntityStatus;

    template<typename T>
    ComponentPool<T> *GetPool() {
        const auto type = std::type_index(typeid(T));
        auto [it, inserted] = m_ComponentPools.try_emplace(type);
        if (inserted) {
            it->second = std::make_unique<ComponentPool<T> >();
        }
        return static_cast<ComponentPool<T> *>(it->second.get());
    }

public:
    Registry() {
        m_EntityStatus.resize(MAX_ENTITIES, false);
        for (EntityID i = 0; i < MAX_ENTITIES; ++i) {
            m_AvailableEntities.push(i);
        }
    }

    bool IsValid(const EntityID id) const {
        if (id >= MAX_ENTITIES)
            return false;
        return m_EntityStatus[id];
    }

    EntityID CreateEntity() {
        assert(m_LivingEntities.size() < MAX_ENTITIES && "Too many entities");
        const EntityID id = m_AvailableEntities.front();
        m_AvailableEntities.pop();
        m_LivingEntities.push_back(id);
        m_EntityStatus[id] = true; // mark as alive
        return id;
    }

    void DestroyEntity(const EntityID entity) {
        for (const auto &pool: m_ComponentPools | std::views::values) {
            pool->EntityDestroyed(entity);
        }
        std::erase(m_LivingEntities, entity);
        m_EntityNames.erase(entity);
        m_AvailableEntities.push(entity);
        m_EntityStatus[entity] = false;
    }

    template<typename T>
    void RemoveComponent(const EntityID entity) { GetPool<T>()->EntityDestroyed(entity); }

    template<typename T, typename... Args>
    T &AddComponent(EntityID entity, Args &&... args) {
        assert(IsValid(entity) && "Attempted to add component to an invalid entity");
        return GetPool<T>()->Emplace(entity, std::forward<Args>(args)...);
    }

    template<typename T>
    T *GetComponent(EntityID entity) {
        if (!IsValid(entity)) {
            return nullptr;
        }
        return GetPool<T>()->Get(entity);
    }

    template<typename T>
    bool HasComponent(EntityID entity) { return GetPool<T>()->Has(entity); }

    void SetEntityName(const EntityID id, const std::string &name) { m_EntityNames[id] = name; }

    const std::string &GetEntityName(const EntityID id) const {
        if (const auto it = m_EntityNames.find(id); it != m_EntityNames.end()) {
            return it->second;
        }
        static std::string empty;
        return empty;
    }

    const std::vector<EntityID> &GetLivingEntities() const { return m_LivingEntities; }

    template<typename Primary, typename... Rest, typename Func>
    void ForEach(Func &&func) {
        for (auto *primaryPool = GetPool<Primary>(); EntityID id: primaryPool->GetDenseEntities()) {
            if ((HasComponent<Rest>(id) && ...)) {
                func(Entity(id, this), primaryPool->Get(id), GetComponent<Rest>(id)...);
            }
        }
    }
};

template<typename T, typename... Args>
T &Entity::AddComponent(Args &&... args) {
    return m_Registry->AddComponent<T>(m_EntityHandle, std::forward<Args>(args)...);
}

template<typename T>
T *Entity::GetComponent() const { return m_Registry->GetComponent<T>(m_EntityHandle); }

template<typename T>
bool Entity::HasComponent() const { return m_Registry->HasComponent<T>(m_EntityHandle); }

template<typename T>
void Entity::RemoveComponent() const { m_Registry->RemoveComponent<T>(m_EntityHandle); }

inline void Entity::SetName(const std::string &name) const { m_Registry->SetEntityName(m_EntityHandle, name); }

inline const std::string &Entity::GetName() const { return m_Registry->GetEntityName(m_EntityHandle); }
