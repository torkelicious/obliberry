#include "Scene.h"
#include "ECS/Systems/MovementSystem.h"
#include "ECS/Systems/RenderSystem.h"
#include "ECS/Systems/InteractionSystem.h"
#include "ECS/Systems/PlayerControlSystem.h"
#include "ECS/Systems/AISystem.h"
#include "ECS/Systems/MapRenderSystem.h"
#include "ECS/Systems/SpriteBillboardSystem.h"
#include "IO/SceneSerialization.h"
#include "IO/EntityFactory.h"
#include <iostream>

Scene::Scene(const EngineContext &context, std::string scenePath)
    : m_Context(context), m_ScenePath(std::move(scenePath)) {
}

void Scene::OnEnter() {
    if (m_Context.renderer) {
        m_Context.renderer->Clean();
    }

    EntityFactory::RegisterDeserializers();
    EntityFactory::RegisterSerializers();
    if (!SceneIO::Deserialize(m_ScenePath, *this)) {
        std::cerr << "Scene: Failed to load scene file: " << m_ScenePath << "\n";
    }
}

void Scene::Update(float dt) {
    m_Context.deltaTime = dt;
    glm::vec2 worldPos = InteractionSystem::Update(m_Registry, m_Context);
    PlayerControlSystem::Update(m_Registry, m_Context, worldPos);
    AISystem::Update(m_Registry, dt);
    MovementSystem::Update(m_Registry, dt);
}

void Scene::Render() {
    MapRenderSystem::RenderTiles(m_Registry, *m_Context.renderer);
    MapRenderSystem::RenderOverlays(m_Registry, *m_Context.renderer);
    if (m_Context.camera) {
        SpriteBillboardSystem::Update(m_Registry, m_Context.camera);
    }
    RenderSystem::Render(m_Registry, *m_Context.renderer);
    m_Context.renderer->Flush();
}
