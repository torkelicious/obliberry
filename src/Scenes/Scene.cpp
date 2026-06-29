#include "Scene.h"
#include "ECS/Systems/AISystem.h"
#include "ECS/Systems/MapRenderSystem.h"
#include "ECS/Systems/MovementSystem.h"
#include "ECS/Systems/PlayerControlSystem.h"
#include "ECS/Systems/RenderSystem.h"
#include "ECS/Systems/SpriteBillboardSystem.h"
#include "ECS/Systems/LightingSystem.h"
#include "IO/EntityFactory.h"
#include "IO/SceneSerialization.h"
#include <iostream>
#include <utility>
#include "ECS/Systems/ScriptSystem.h"
#include "IO/PrefabManager.h"
#include "Math/Frustum.h"
#include "Scripting/EngineLib/EngineLib.h"
#include "Sound/AudioEngine.h"

Scenes::Scene::Scene(Core::EngineContext context, SceneProperties props)
    : m_Properties(std::move(props)), m_Context(std::move(context)) {
}

void Scenes::Scene::OnEnter() {
    Scripting::EngineLib::EngineLib lib;
    lib.register_enginelib(*m_Context.scriptEngine, m_Registry, m_Context);

    IO::EntityFactory::RegisterDeserializers();
    IO::EntityFactory::RegisterSerializers();

    if (!IO::SceneIO::Deserialize(m_Properties.ScenePath, *this)) {
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
        Rendering::Renderer::SetClearColor(m_Properties.BackgroundClearColor);
    }
}

void Scenes::Scene::Update(const float dt) {
    m_Context.deltaTime = dt;

    ECS::Systems::PlayerControlSystem::Update(m_Registry, m_Context);
    ECS::Systems::AISystem::Update(m_Registry, dt);
    ECS::Systems::MovementSystem::Update(m_Registry, dt);
    ECS::Systems::ScriptSystem::Update(m_Registry, m_Context);
    ECS::Systems::LightingSystem::Update(m_Registry);
}

void Scenes::Scene::Render() {
    m_Context.renderer->BeginFrame();

    const glm::mat4 &vp = m_Context.renderer->GetCurrentVP();

    if (m_Context.camera) {
        const Math::Frustum::ViewFrustum frustum =
                Math::Frustum::FromCameraVP(vp, /*padding=*/ Core::HEX_SIZE * 2.0f);
        const Math::Frustum::FrustumPlanes frustum3D =
                Math::Frustum::FrustumPlanes::FromVP(vp);

        ECS::Systems::MapRenderSystem::RenderAll(m_Registry, m_Context, frustum);

        ECS::Systems::SpriteBillboardSystem::Update(m_Registry, m_Context.camera);

        ECS::Systems::RenderSystem::Render(m_Registry, *m_Context.renderer, frustum3D);
    }
}

void Scenes::Scene::OnExit() {
    std::vector<ECS::EntityID> deadEntities;
    m_Registry.ForEach<ECS::Components::DestroyTagComponent>(
        [&](const ECS::Entity entity, ECS::Components::DestroyTagComponent *) {
            deadEntities.push_back(static_cast<ECS::EntityID>(entity));
        });

    for (const ECS::EntityID id: deadEntities) {
        if (m_Registry.IsValid(id)) {
            m_Registry.DestroyEntity(id);
        }
    }

    ECS::Systems::ScriptSystem::OnSceneExit(m_Registry, m_Context);
    if (m_Context.renderer) {
        m_Context.renderer->Clean();
    }
    IO::PrefabManager::ClearCache();
    std::cout << "Exiting Scene " << m_Properties.ScenePath << "\n";
}
