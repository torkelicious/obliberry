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

    Registry &GetRegistry() { return m_Registry; }

    EngineContext m_Context;
    Registry m_Registry;
};

#endif //OBLIBERRY_SCENE_H
