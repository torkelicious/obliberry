#ifndef OBLIBERRY_SCENE_H
#define OBLIBERRY_SCENE_H

#include "Core/EngineContext.h"
#include "ECS/ECS.h"
#include "Renderer/Renderer.h"

class Scene {
public:
    Scene(const EngineContext &context);

    ~Scene() = default;

    void OnEnter();

    void OnExit() {
    }

    void Update(float dt);

    void Render(Renderer &renderer);

    [[nodiscard]] Registry &GetRegistry() { return m_Registry; }
    [[nodiscard]] const Registry &GetRegistry() const { return m_Registry; }
    [[nodiscard]] EngineContext &GetContext() { return m_Context; }
    [[nodiscard]] const EngineContext &GetContext() const { return m_Context; }

private:
    EngineContext m_Context;
    Registry m_Registry;
};

#endif //OBLIBERRY_SCENE_H
