#ifndef OBLIBERRY_ENTITY_H
#define OBLIBERRY_ENTITY_H

#include "Types.h"
#include "Registry.h"
#include <utility>

class Entity {
public:
    Entity() = default;

    Entity(EntityID handle, Registry *registry)
        : m_EntityHandle(handle), m_Registry(registry) {
    }

    template<typename T, typename... Args>
    T &AddComponent(Args &&... args) {
        return m_Registry->AddComponent<T>(m_EntityHandle, std::forward<Args>(args)...);
    }

    template<typename T>
    T *GetComponent() const {
        return m_Registry->GetComponent<T>(m_EntityHandle);
    }

    template<typename T>
    bool HasComponent() const {
        return m_Registry->HasComponent<T>(m_EntityHandle);
    }

    // comp ops
    bool operator==(const Entity &other) const {
        return m_EntityHandle == other.m_EntityHandle && m_Registry == other.m_Registry;
    }

    bool operator!=(const Entity &other) const {
        return !(*this == other);
    }

    operator bool() const { return m_EntityHandle != 0; }
    operator EntityID() const { return m_EntityHandle; }

private:
    EntityID m_EntityHandle = 0;
    Registry *m_Registry = nullptr;
};


#endif //OBLIBERRY_ENTITY_H
