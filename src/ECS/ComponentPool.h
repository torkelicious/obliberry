#ifndef OBLIBERRY_COMPONENTPOOL_H
#define OBLIBERRY_COMPONENTPOOL_H
#include "IPool.h"
#include "Types.h"
#include <vector>
#include <limits>
#include <cassert>
#include <utility>

template<typename T>
class ComponentPool : public IPool {
private:
    std::vector<T> m_Data; // array of components
    std::vector<EntityID> m_IndexToEntity; // array mapping index back to EntityID
    std::vector<size_t> m_EntityToIndex; // array mapping EntityID to Index
    static constexpr size_t INVALID_INDEX = std::numeric_limits<size_t>::max();

public:
    ComponentPool() {
        // pre-allocate
        m_EntityToIndex.resize(MAX_ENTITIES, INVALID_INDEX);
    }

    T &Insert(const EntityID entity, T component) {
        assert(entity < MAX_ENTITIES && "Entity ID exceeds maximum limit!");

        const size_t newIndex = m_Data.size();
        m_EntityToIndex[entity] = newIndex;
        m_IndexToEntity.push_back(entity);
        m_Data.push_back(std::move(component));

        return m_Data.back();
    }

    T *Get(const EntityID entity) {
        if (entity >= m_EntityToIndex.size() || m_EntityToIndex[entity] == INVALID_INDEX) {
            return nullptr;
        }
        // fast :))))
        return &m_Data[m_EntityToIndex[entity]];
    }

    const T *Get(const EntityID entity) const {
        if (entity >= m_EntityToIndex.size() || m_EntityToIndex[entity] == INVALID_INDEX) {
            return nullptr;
        }
        return &m_Data[m_EntityToIndex[entity]];
    }

    [[nodiscard]] bool Has(const EntityID entity) const {
        return entity < m_EntityToIndex.size() && m_EntityToIndex[entity] != INVALID_INDEX;
    }

    void EntityDestroyed(const EntityID entity) override {
        if (!Has(entity)) return;

        const size_t indexOfRemoved = m_EntityToIndex[entity];
        const size_t indexOfLast = m_Data.size() - 1;

        if (indexOfRemoved != indexOfLast) {
            m_Data[indexOfRemoved] = std::move(m_Data[indexOfLast]);
            const EntityID entityOfLast = m_IndexToEntity[indexOfLast];
            m_IndexToEntity[indexOfRemoved] = entityOfLast;
            m_EntityToIndex[entityOfLast] = indexOfRemoved;
        }

        m_EntityToIndex[entity] = INVALID_INDEX;
        m_IndexToEntity.pop_back();
        m_Data.pop_back();
    }

    template<typename... Args>
    T &Emplace(const EntityID entity, Args &&... args) {
        return Insert(entity, T(std::forward<Args>(args)...));
    }

    // expose if needed l8r
    const std::vector<T> &GetDenseData() const { return m_Data; }
    [[nodiscard]] const std::vector<EntityID> &GetDenseEntities() const { return m_IndexToEntity; }
};

#endif //OBLIBERRY_COMPONENTPOOL_H
