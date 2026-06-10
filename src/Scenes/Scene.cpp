#include "Scene.h"
#include "ECS/Systems/MovementSystem.h"
#include "ECS/Systems/RenderSystem.h"
#include "ECS/Systems/InteractionSystem.h"
#include "ECS/Systems/PlayerControlSystem.h"
#include "ECS/Systems/AISystem.h"
#include "ECS/Systems/MapRenderSystem.h"
#include "ECS/Systems/SpriteBillboardSystem.h"
#include "IO/SceneSerializer.h"
#include "IO/EntityFactory.h"
#include <iostream>

Scene::Scene(const EngineContext &context) : m_Context(context) {
}

void Scene::OnEnter() {
    EntityFactory::RegisterDeserializers();
    EntityFactory::RegisterSerializers();
    if (!SceneIO::Deserialize("assets/scenes/level01.json", *this)) {
        std::cerr << "Scene: Failed to load scene file!\n";
        return;
    }
}

void Scene::Update(float dt) {
    m_Context.deltaTime = dt;
    glm::vec2 worldPos = InteractionSystem::Update(m_Registry, m_Context);
    PlayerControlSystem::Update(m_Registry, m_Context, worldPos);
    AISystem::Update(m_Registry, dt);
    MovementSystem::Update(m_Registry, dt);
}

void Scene::Render(Renderer &renderer) {
    MapRenderSystem::RenderTiles(m_Registry, renderer);
    MapRenderSystem::RenderOverlays(m_Registry, renderer);
    if (m_Context.camera) {
        SpriteBillboardSystem::Update(m_Registry, m_Context.camera);
    }
    RenderSystem::Render(m_Registry, renderer);
    renderer.Flush();
}
