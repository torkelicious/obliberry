#pragma once

#include "IPool.h"
#include "Types.h"
#include <vector>
#include <limits>
#include <cassert>
#include <utility>

namespace ECS {
    template <typename T> class ComponentPool : public IPool {
        std::vector<T> m_Data;
        std::vector<EntityID> m_IndexToEntity;
        std::vector<uint32_t> m_EntityToIndex;
        static constexpr uint32_t INVALID_INDEX = std::numeric_limits<uint32_t>::max();

    public:
        ComponentPool() { m_EntityToIndex.resize(MAX_ENTITIES, INVALID_INDEX); }

        T &Insert(const EntityID entity, T component) {
            const uint32_t index = GetEntityIndex(entity);
            assert(index < MAX_ENTITIES && "Entity ID exceeds maximum limit!");

            const auto newIndex = static_cast<uint32_t>(m_Data.size());
            m_EntityToIndex[index] = newIndex;
            m_IndexToEntity.push_back(entity); // Keep full ID for reverse lookup
            m_Data.push_back(std::move(component));

            return m_Data.back();
        }

        T *Get(const EntityID entity) {
            const uint32_t index = GetEntityIndex(entity);
            if (index >= m_EntityToIndex.size() || m_EntityToIndex[index] == INVALID_INDEX) {
                return nullptr;
            }
            return &m_Data[m_EntityToIndex[index]];
        }

        [[nodiscard]] bool Has(const EntityID entity) const {
            const uint32_t index = GetEntityIndex(entity);
            return index < m_EntityToIndex.size() && m_EntityToIndex[index] != INVALID_INDEX;
        }

        void EntityDestroyed(const EntityID entity) override {
            if (!Has(entity))
                return;

            const uint32_t entityIdx = GetEntityIndex(entity);
            const uint32_t indexOfRemoved = m_EntityToIndex[entityIdx];
            const uint32_t indexOfLast = static_cast<uint32_t>(m_Data.size()) - 1;

            if (indexOfRemoved != indexOfLast) {
                m_Data[indexOfRemoved] = std::move(m_Data[indexOfLast]);
                const EntityID entityOfLast = m_IndexToEntity[indexOfLast];
                m_IndexToEntity[indexOfRemoved] = entityOfLast;
                // Update the index mapping for the swapped entity
                m_EntityToIndex[GetEntityIndex(entityOfLast)] = indexOfRemoved;
            }

            m_EntityToIndex[entityIdx] = INVALID_INDEX;
            m_IndexToEntity.pop_back();
            m_Data.pop_back();
        }

        template <typename... Args> T &Emplace(const EntityID entity, Args &&...args) { return Insert(entity, T(std::forward<Args>(args)...)); }

        [[nodiscard]] const std::vector<T> &GetDenseData() const { return m_Data; }
        [[nodiscard]] const std::vector<EntityID> &GetDenseEntities() const { return m_IndexToEntity; }
    };
} // namespace ECS
