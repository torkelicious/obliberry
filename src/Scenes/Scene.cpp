#include "Scene.h"
#include "ECS/Systems/MovementSystem.h"
#include "ECS/Systems/PlayerInputSystem.h"
#include "ECS/Systems/RenderSystem.h"
#include "ECS/Systems/InteractionSystem.h"
#include "ECS/Systems/PlayerControlSystem.h"
#include "ECS/Systems/AISystem.h"
#include "ECS/Systems/MapRenderSystem.h"
#include "ECS/Systems/SpriteBillboardSystem.h"
#include "IO/SceneSerializer.h"
#include "IO/EntityFactory.h"
#include "ECS/Components/MapComponent.h"
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
    // visual resources
    auto shader = m_Context.resources->Get<Shader>("base_shader");
    auto grassTex = m_Context.resources->Get<Texture>("grass_tex");
    auto sandTex = m_Context.resources->Get<Texture>("sand_tex");
    auto hexMesh = m_Context.resources->Get<Mesh>("hex_mesh");
    // find map entity created by the serializer and attach visual assets
    m_Registry.ForEach<MapComponent>([&](Entity, MapComponent *mapComp) {
        mapComp->hexMesh = hexMesh;
        if (shader && grassTex && sandTex && hexMesh) {
            mapComp->grassMat = {shader, grassTex, {1, 1, 1, 1}};
            mapComp->sandMat = {shader, sandTex, {1, 1, 1, 1}};
            mapComp->outlineMat = {shader, nullptr, {1, 0, 0, 0.5f}};
            mapComp->pathToMat = {shader, nullptr, {1, 1, 1, 0.5f}};
        } else {
            std::cerr << "Scene: Missing map visual assets!\n";
        }
    });
}

void Scene::Update(float dt) {
    m_Context.deltaTime = dt;
    glm::vec2 worldPos = InteractionSystem::Update(m_Registry, m_Context);
    PlayerControlSystem::Update(m_Registry, m_Context, worldPos);
    AISystem::Update(m_Registry);
    InputSystem::Update(m_Registry, *m_Context.input);
    MovementSystem::Update(m_Registry, dt);
}

void Scene::Render(Renderer &renderer) {
    MapRenderSystem::Render(m_Registry, renderer);
    if (m_Context.camera) {
        SpriteBillboardSystem::Update(m_Registry, m_Context.camera);
    }
    RenderSystem::Render(m_Registry, renderer);
}
