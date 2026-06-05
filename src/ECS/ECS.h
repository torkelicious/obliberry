#ifndef ISOMETRICGAME_ECS_H
#define ISOMETRICGAME_ECS_H
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
    Entity();

    ~Entity();

    template<typename T, typename... Args>
    T *AddComponent(Args &&... args) {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
        T *component = new T(std::forward<Args>(args)...);
        m_Components.emplace_back(std::unique_ptr<Component>(component));
        return component;
    }

    template<typename T>
    T *GetComponent() {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
        for (const auto &componentPtr: m_Components) {
            if (dynamic_cast<T *>(componentPtr.get())) {
                return static_cast<T *>(componentPtr.get());
            }
        }
        return nullptr;
    }

    // Implement systems later for stuff i guess
    void Update(float dt);

    // find all render methods in our components ig
    void Render();

private:
    std::vector<std::unique_ptr<Component> > m_Components;
};


#endif //ISOMETRICGAME_ECS_H
