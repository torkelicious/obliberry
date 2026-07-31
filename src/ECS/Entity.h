#pragma once

#include "Types.h"
#include <string>
#include <vector>

namespace ECS {
    class Registry;

    class Entity {
    public:
        Entity() = default;

        Entity(const EntityID handle, Registry *registry) : m_EntityHandle(handle), m_Registry(registry) {
        }

        template <typename T, typename... Args> T &AddComponent(Args &&... args);

        template <typename T> T *GetComponent() const;

        template <typename T> [[nodiscard]] bool HasComponent() const;

        template <typename T> void RemoveComponent() const;

        bool operator==(const Entity &other) const { return m_EntityHandle == other.m_EntityHandle && m_Registry == other.m_Registry; }

        bool operator!=(const Entity &other) const { return !(*this == other); }
        explicit operator bool() const { return m_EntityHandle != 0; }
        explicit operator EntityID() const { return m_EntityHandle; }

        // basically onlt for ui stuff / gameplay
        // all engine logic handles via ID handles.
        void SetName(const std::string &name) const;

        [[nodiscard]] const std::string &GetName() const;

        // hierarchy
        void SetParent(EntityID parentId) const;
        [[nodiscard]] Entity GetParent() const;
        [[nodiscard]] const std::vector<EntityID> &GetChildren() const;

    private:
        EntityID m_EntityHandle = 0;
        Registry *m_Registry = nullptr;
    };
} // namespace ECS
