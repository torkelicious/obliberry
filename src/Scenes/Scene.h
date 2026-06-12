#ifndef OBLIBERRY_SCENE_H
#define OBLIBERRY_SCENE_H

#include <string>
#include <glm/glm.hpp>
#include "Core/EngineContext.h"
#include "ECS/ECS.h"

struct SceneProperties {
    std::string ScenePath;
    std::string Name; // unused 4 now
    glm::vec4 BackgroundClearColor = {0, 0, 0, 1};
    float AmbientLight = 0.2f;
    // todo: bg music
};

class Scene {
public:
    Scene(const EngineContext &context, SceneProperties props);

    ~Scene() = default;

    void OnEnter();

    void OnExit() const;

    void Update(float dt);

    void Render();

    [[nodiscard]] Registry &GetRegistry() { return m_Registry; }
    [[nodiscard]] const Registry &GetRegistry() const { return m_Registry; }
    [[nodiscard]] EngineContext &GetContext() { return m_Context; }
    [[nodiscard]] const EngineContext &GetContext() const { return m_Context; }

    [[nodiscard]] const std::string &GetScenePath() const {
        return m_Properties.ScenePath;
    }

    SceneProperties &GetProperties() { return m_Properties; };

private:
    SceneProperties m_Properties;
    EngineContext m_Context;
    Registry m_Registry;
};

#endif //OBLIBERRY_SCENE_H
