#include "Scene.h"
#include "ECS/Systems/MovementSystem.h"
#include "ECS/Systems/RenderSystem.h"
#include "ECS/Systems/InteractionSystem.h"
#include "ECS/Systems/PlayerControlSystem.h"
#include "ECS/Systems/AISystem.h"
#include "ECS/Systems/MapRenderSystem.h"
#include "ECS/Systems/SpriteBillboardSystem.h"
#include "ECS/Systems/lightingSystem.h"
#include "IO/SceneSerialization.h"
#include "IO/EntityFactory.h"
#include <iostream>

Scene::Scene(const EngineContext &context, SceneProperties props)
    : m_Properties(std::move(props)), m_Context(context) {
}

void Scene::OnEnter() {
    EntityFactory::RegisterDeserializers();
    EntityFactory::RegisterSerializers();

    if (!SceneIO::Deserialize(m_Properties.ScenePath, *this)) {
        std::cerr << "Scene: Failed to load scene file: " << m_Properties.ScenePath << "\n";
    }

    if (m_Context.renderer) {
        m_Context.renderer->Clean();
        m_Context.renderer->SetClearColor(m_Properties.BackgroundClearColor);
    }
}

void Scene::Update(const float dt) {
    m_Context.deltaTime = dt;
    // this looks stupid but is fine because Update checks for required component for system before running
    const glm::vec2 worldPos = InteractionSystem::Update(m_Registry, m_Context);
    PlayerControlSystem::Update(m_Registry, m_Context, worldPos);
    AISystem::Update(m_Registry, dt);
    MovementSystem::Update(m_Registry, dt);
    LightingSystem::Update(m_Registry);
}

void Scene::Render() {
    MapRenderSystem::RenderAll(m_Registry, m_Context);
    if (m_Context.camera) {
        SpriteBillboardSystem::Update(m_Registry, m_Context.camera);
    }
    RenderSystem::Render(m_Registry, *m_Context.renderer);
    m_Context.renderer->InstancedFlush();
    m_Context.renderer->Flush();
}

void Scene::OnExit() {
    if (m_Context.renderer) {
        m_Context.renderer->Clean();
    }
    std::cout << "Exiting Scene " << m_Properties.ScenePath << "\n";
}
