

#ifndef OBLIBERRY_COMPONENTPOOL_H
#define OBLIBERRY_COMPONENTPOOL_H

#include "IPool.h"
#include "Types.h"
#include <vector>
#include <unordered_map>

template<typename T>
class ComponentPool : public IPool {
private:
    std::vector<T> m_Data; // arr
    std::unordered_map<EntityID, size_t> m_EntityToIndex;
    std::unordered_map<size_t, EntityID> m_IndexToEntity;

public:
    T &Insert(EntityID entity, T component) {
        size_t newIndex = m_Data.size();
        m_EntityToIndex[entity] = newIndex;
        m_IndexToEntity[newIndex] = entity;
        m_Data.push_back(component);
        return m_Data.back();
    }

    T *Get(EntityID entity) {
        if (m_EntityToIndex.find(entity) == m_EntityToIndex.end()) {
            return nullptr;
        }
        return &m_Data[m_EntityToIndex[entity]];
    }

    bool Has(EntityID entity) {
        return m_EntityToIndex.find(entity) != m_EntityToIndex.end();
    }

    void EntityDestroyed(EntityID entity) override {
        if (m_EntityToIndex.find(entity) == m_EntityToIndex.end()) { return; }

        // keep dense via swap
        size_t indexOfRemoved = m_EntityToIndex[entity];
        size_t indexOfLastEl = m_Data.size() - 1;
        m_Data[indexOfRemoved] = m_Data[indexOfLastEl];

        EntityID entityOfLastEl = m_IndexToEntity[indexOfLastEl];
        m_EntityToIndex[entityOfLastEl] = indexOfRemoved;
        m_IndexToEntity[indexOfRemoved] = entityOfLastEl;

        m_EntityToIndex.erase(entity);
        m_IndexToEntity.erase(indexOfLastEl);
        m_Data.pop_back();
    }
};

#endif //OBLIBERRY_COMPONENTPOOL_H
