#ifndef OBLIBERRY_REGISTRY_H
#define OBLIBERRY_REGISTRY_H

#include "Types.h"
#include "ComponentPool.h"
#include <queue>
#include <unordered_map>
#include <memory>
#include <typeindex>
#include <cassert>

#include "Entity.h"

class Registry {
private:
    std::queue<EntityID> m_AvailableEntities;
    uint32_t m_LivingEntityCount = 0;
    std::unordered_map<std::type_index, std::unique_ptr<IPool> > m_ComponentPools;
    std::vector<EntityID> m_LivingEntities;

    template<typename T>
    ComponentPool<T> *GetPool() {
        auto type = std::type_index(typeid(T));
        if (m_ComponentPools.find(type) == m_ComponentPools.end()) {
            m_ComponentPools[type] = std::make_unique<ComponentPool<T> >();
        }
        return static_cast<ComponentPool<T> *>(m_ComponentPools[type].get());
    }

public:
    Registry() {
        for (EntityID i = 0; i < MAX_ENTITIES; ++i) {
            m_AvailableEntities.push(i);
        }
    }

    EntityID CreateEntity() {
        assert(m_LivingEntityCount < MAX_ENTITIES && "Too many entities");
        EntityID id = m_AvailableEntities.front();
        m_AvailableEntities.pop();
        m_LivingEntityCount++;
        m_LivingEntities.push_back(id);
        return id;
    }

    void DestroyEntity(EntityID entity) {
        for (auto const &[type, pool]: m_ComponentPools) {
            pool->EntityDestroyed(entity);
        }
        std::erase(m_LivingEntities, entity);

        m_AvailableEntities.push(entity);
        m_LivingEntityCount--;
    }

    //template<typename T>
    //T &AddComponent(EntityID entity, T component) { return GetPool<T>()->Insert(entity, component); }
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
