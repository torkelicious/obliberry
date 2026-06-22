#include "Scene.h"
#include "ECS/Systems/AISystem.h"
#include "ECS/Systems/InteractionSystem.h"
#include "ECS/Systems/MapRenderSystem.h"
#include "ECS/Systems/MovementSystem.h"
#include "ECS/Systems/PlayerControlSystem.h"
#include "ECS/Systems/RenderSystem.h"
#include "ECS/Systems/SpriteBillboardSystem.h"
#include "ECS/Systems/lightingSystem.h"
#include "IO/EntityFactory.h"
#include "IO/SceneSerialization.h"
#include <iostream>
#include <utility>
#include "ECS/Systems/ScriptSystem.h"
#include "IO/PrefabManager.h"
#include "Scripting/EngineLib/EngineLib.h"
#include "Sound/AudioEngine.h"

Scene::Scene(EngineContext context, SceneProperties props)
    : m_Properties(std::move(props)), m_Context(std::move(context)) {
}

void Scene::OnEnter() {
    EngineLib lib;
    lib.register_enginelib(*m_Context.scriptEngine, m_Registry, m_Context);

    EntityFactory::RegisterDeserializers();
    EntityFactory::RegisterSerializers();

    if (!SceneIO::Deserialize(m_Properties.ScenePath, *this)) {
        std::cerr << "Scene: Failed to load scene file: " << m_Properties.ScenePath
                << "\n";
    }

    if (m_Context.audioEngine) {
        if (!m_Properties.BackgroundMusicPath.empty()) {
            m_Context.audioEngine->PlayMusic(m_Properties.BackgroundMusicPath);
        } else {
            m_Context.audioEngine->StopMusic();
        }
    }

    if (m_Context.renderer) {
        m_Context.renderer->Clean();
        Renderer::SetClearColor(m_Properties.BackgroundClearColor);
    }
}

void Scene::Update(const float dt) {
    m_Context.deltaTime = dt;
    // this looks stupid but is fine because Update checks for required component
    // for system before running
    const glm::vec2 worldPos = InteractionSystem::Update(m_Registry, m_Context);
    PlayerControlSystem::Update(m_Registry, worldPos);
    AISystem::Update(m_Registry, dt);
    MovementSystem::Update(m_Registry, dt);
    ScriptSystem::Update(m_Registry, m_Context);
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
    std::vector<EntityID> deadEntities;
    m_Registry.ForEach<DestroyTagComponent>(
        [&](const Entity entity, DestroyTagComponent *) {
            deadEntities.push_back(static_cast<EntityID>(entity));
        });

    for (const EntityID id: deadEntities) {
        if (m_Registry.IsValid(id)) {
            m_Registry.DestroyEntity(id);
        }
    }

    ScriptSystem::OnSceneExit(m_Registry, m_Context);
    if (m_Context.renderer) {
        m_Context.renderer->Clean();
    }
    PrefabManager::ClearCache();
    std::cout << "Exiting Scene " << m_Properties.ScenePath << "\n";
}
