#ifndef OBLIBERRY_ENTITY_H
#define OBLIBERRY_ENTITY_H

#include "Types.h"

class Registry;

class Entity {
public:
    Entity() = default;

    Entity(const EntityID handle, Registry *registry)
        : m_EntityHandle(handle), m_Registry(registry) {
    }

    template<typename T, typename... Args>
    T &AddComponent(Args &&... args);

    template<typename T>
    T *GetComponent() const;

    template<typename T>
    [[nodiscard]] bool HasComponent() const;

    bool operator==(const Entity &other) const {
        return m_EntityHandle == other.m_EntityHandle && m_Registry == other.m_Registry;
    }

    bool operator!=(const Entity &other) const { return !(*this == other); }
    explicit operator bool() const { return m_EntityHandle != 0; }
    explicit operator EntityID() const { return m_EntityHandle; }

private:
    EntityID m_EntityHandle = 0;
    Registry *m_Registry = nullptr;
};


#endif //OBLIBERRY_ENTITY_H
