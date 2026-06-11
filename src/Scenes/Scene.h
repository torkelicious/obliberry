#ifndef OBLIBERRY_SCENE_H
#define OBLIBERRY_SCENE_H

#include <string>
#include "Core/EngineContext.h"
#include "ECS/ECS.h"
#include "Renderer/Renderer.h"

class Scene {
public:
    Scene(const EngineContext &context, std::string scenePath);

    ~Scene() = default;

    void OnEnter();

    void OnExit() {
    }

    void Update(float dt);

    void Render();

    [[nodiscard]] Registry &GetRegistry() { return m_Registry; }
    [[nodiscard]] const Registry &GetRegistry() const { return m_Registry; }
    [[nodiscard]] EngineContext &GetContext() { return m_Context; }
    [[nodiscard]] const EngineContext &GetContext() const { return m_Context; }
    [[nodiscard]] const std::string &GetScenePath() const { return m_ScenePath; }

private:
    EngineContext m_Context;
    Registry m_Registry;
    std::string m_ScenePath;
};

#endif //OBLIBERRY_SCENE_H
