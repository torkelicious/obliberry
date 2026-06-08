

#ifndef OBLIBERRY_REGISTRY_H
#define OBLIBERRY_REGISTRY_H

#include "Types.h"
#include "ComponentPool.h"
#include <queue>
#include <unordered_map>
#include <memory>
#include <typeindex>
#include <cassert>

class Registry {
private:
    std::queue<EntityID> m_AvailableEntities;
    uint32_t m_LivingEntityCount = 0;
    std::unordered_map<std::type_index, std::unique_ptr<IPool> > m_ComponentPools;

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
        return id;
    }

    void DestroyEntity(EntityID entity) {
        for (auto const &[type, pool]: m_ComponentPools) {
            pool->EntityDestroyed(entity);
        }
        m_AvailableEntities.push(entity);
        m_LivingEntityCount--;
    }

    template<typename T>
    T &AddComponent(EntityID entity, T component) {
        return GetPool<T>()->Insert(entity, component);
    }

    template<typename T>
    T *GetComponent(EntityID entity) {
        return GetPool<T>()->Get(entity);
    }

    template<typename T>
    bool HasComponent(EntityID entity) {
        return GetPool<T>()->Has(entity);
    }
};

#endif //OBLIBERRY_REGISTRY_H
