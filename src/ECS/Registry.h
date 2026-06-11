#ifndef OBLIBERRY_REGISTRY_H
#define OBLIBERRY_REGISTRY_H

#include "Types.h"
#include "ComponentPool.h"
#include <queue>
#include <unordered_map>
#include <memory>
#include <typeindex>
#include <cassert>
#include <ranges>

#include "Entity.h"

class Registry {
private:
    std::queue<EntityID> m_AvailableEntities;
    std::unordered_map<std::type_index, std::unique_ptr<IPool> > m_ComponentPools;
    std::vector<EntityID> m_LivingEntities;

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
        for (EntityID i = 0; i < MAX_ENTITIES; ++i) {
            m_AvailableEntities.push(i);
        }
    }

    EntityID CreateEntity() {
        assert(m_LivingEntities.size() < MAX_ENTITIES && "Too many entities");
        const EntityID id = m_AvailableEntities.front();
        m_AvailableEntities.pop();
        m_LivingEntities.push_back(id);
        return id;
    }

    void DestroyEntity(const EntityID entity) {
        for (const auto &pool: m_ComponentPools | std::views::values) {
            pool->EntityDestroyed(entity);
        }
        std::erase(m_LivingEntities, entity);

        m_AvailableEntities.push(entity);
    }

    template<typename T, typename... Args>
    T &AddComponent(EntityID entity, Args &&... args) {
        return GetPool<T>()->Emplace(entity, std::forward<Args>(args)...);
    }

    template<typename T>
    T *GetComponent(EntityID entity) { return GetPool<T>()->Get(entity); }

    template<typename T>
    bool HasComponent(EntityID entity) { return GetPool<T>()->Has(entity); }

    const std::vector<EntityID> &GetLivingEntities() const { return m_LivingEntities; }

    template<typename Primary, typename... Rest, typename Func>
    void ForEach(Func &&func) {
        auto *primaryPool = GetPool<Primary>();
        for (EntityID id: primaryPool->GetDenseEntities()) {
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
T *Entity::GetComponent() const {
    return m_Registry->GetComponent<T>(m_EntityHandle);
}

template<typename T>
bool Entity::HasComponent() const {
    return m_Registry->HasComponent<T>(m_EntityHandle);
}


#endif //OBLIBERRY_REGISTRY_H
