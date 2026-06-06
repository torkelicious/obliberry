#ifndef OBLIBERRY_ECS_H
#define OBLIBERRY_ECS_H
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

// very basic shitty ECS
class Entity;
class Component;

class Component {
public:
    virtual ~Component() = default;

    // Implement systems later for stuff i guess
    virtual void UpdateComponent(float dt, Entity &entity) {
    }

    /* Optional:
     * If component has a renderable something(?) add it,
     * to render queue */
    virtual void Render() {
    }
};

class Entity {
public:
    Entity() = default;

    ~Entity() = default;

    template<typename T, typename... Args>
    T *AddComponent(Args &&... args) {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
        T *component = new T(std::forward<Args>(args)...);
        m_Components.emplace_back(std::unique_ptr<Component>(component));
        return component;
    }

    template<typename T>
    T *GetComponent() const {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
        for (const auto &componentPtr: m_Components) {
            if (dynamic_cast<T *>(componentPtr.get())) {
                return static_cast<T *>(componentPtr.get());
            }
        }
        return nullptr;
    }

    template<typename T>
    bool HasComponent() const {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
        for (const auto &componentPtr: m_Components) {
            if (dynamic_cast<T *>(componentPtr.get())) {
                return true;
            }
        }
        return false;
    }

    template<typename T>
    void RemoveComponent() {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
        for (auto it = m_Components.begin(); it != m_Components.end(); ++it) {
            if (dynamic_cast<T *>(it->get())) {
                m_Components.erase(it);
                return;
            }
        }
    }

    // Loop through all components and call their update methods
    void Update(float dt) {
        for (auto &componentPtr: m_Components) {
            componentPtr->UpdateComponent(dt, *this);
        }
    }

    // Loop through all components and call their render methods
    void Render() {
        for (auto &componentPtr: m_Components) {
            componentPtr->Render();
        }
    }

private:
    std::vector<std::unique_ptr<Component> > m_Components;
};

#endif //OBLIBERRY_ECS_H