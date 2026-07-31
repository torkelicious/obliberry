#include "Scene.h"
#include "Logger/LoggerService.h"
#include "ECS/Systems/AISystem.h"
#include "ECS/Systems/MapRenderSystem.h"
#include "ECS/Systems/MovementSystem.h"
#include "ECS/Systems/PlayerControlSystem.h"
#include "ECS/Systems/RenderSystem.h"
#include "ECS/Systems/SpriteBillboardSystem.h"
#include "ECS/Systems/LightingSystem.h"
#include "ECS/Systems/ParticleSystem.h"
#include "IO/Loaders/EntityFactory.h"
#include "IO/SceneSerialization.h"
#include <iostream>
#include <utility>
#include "ECS/Systems/ScriptSystem.h"
#include "ECS/Systems/HierarchySystem.h"
#include "ECS/Systems/HierarchySystem.h"
#include "IO/Loaders/PrefabManager.h"
#include "Math/Frustum.h"
#include "Scripting/EngineLib/EngineLib.h"
#include "Sound/AudioEngine.h"

#pragma push_macro("LOG_WHO")
#define LOG_WHO "Scene"

Scenes::Scene::Scene(Core::EngineContext *context, SceneProperties props) : m_Properties(std::move(props)), m_Context(context), m_UISystem(context->uiRenderer, context->input) {
}

void Scenes::Scene::OnEnter() {
    LOG_INFO(LOG_WHO, "Entering scene: " + m_Properties.ScenePath);

    m_Context->uiSystem = &m_UISystem;
    m_Context->uiCmdBuf = &m_UICmdBuf;

    // Register EngineLib native functions on every worker's interpreter
    for (size_t w = 0; w < m_Context->scriptPool->worker_count(); ++w) {
        Scripting::EngineLib lib;
        lib.register_enginelib(m_Context->scriptPool->get_worker(w)->GetInterpreter(), m_Registry, *m_Context);
    }

    IO::EntityFactory::RegisterDeserializers();
    IO::EntityFactory::RegisterSerializers();

    LOG_INFO(LOG_WHO, "Attempting to deserialize scene from: " + m_Properties.ScenePath);
    if (!IO::SceneIO::Deserialize(m_Properties.ScenePath, *this)) {
        LOG_ERROR(LOG_WHO, "Failed to load scene file: " + m_Properties.ScenePath);
        LOG_ERROR(LOG_WHO, "Scene will be empty!");
    } else {
        LOG_INFO(LOG_WHO, "Successfully deserialized scene");
    }

    if (m_Context->audioEngine && !m_Context->isEditorMode) {
        if (!m_Properties.BackgroundMusicPath.empty()) {
            m_Context->audioEngine->PlayMusic(m_Properties.BackgroundMusicPath);
        } else {
            m_Context->audioEngine->StopMusic();
        }
    }

    if (m_Context->renderer) {
        Rendering::Renderer::SetClearColor(m_Properties.BackgroundClearColor);
    }

    if (auto *mapComp = m_Registry.GetFirst<ECS::Components::MapComponent>()) {
        ECS::Systems::LightingSystem::GenerateLightmap(*mapComp, m_Context->resources);
    }

    ECS::Systems::LightingSystem::Update(m_Registry);
}

void Scenes::Scene::Update(const float dt) {
    m_Context->deltaTime = dt;
    m_Context->frameCount++;

    ECS::Systems::HierarchySystem::Propagate(m_Registry);

    if (m_Context->uiSystem) {
        m_Context->uiSystem->Update(dt);
        m_Context->uiSystem->SnapshotButtonStates();
    }

    ECS::Systems::PlayerControlSystem::Update(m_Registry, *m_Context);
    ECS::Systems::AISystem::Update(m_Registry, dt);
    ECS::Systems::MovementSystem::Update(m_Registry, dt);
    ECS::Systems::ScriptSystem::Update(m_Registry, *m_Context);
    m_UICmdBuf.flush(m_UISystem);
    ECS::Systems::LightingSystem::Update(m_Registry);
    ECS::Systems::ParticleSystem::Update(m_Registry, dt);
}

void Scenes::Scene::Render() {
    m_Context->renderer->BeginFrame();

    ECS::Systems::HierarchySystem::Propagate(m_Registry);

    const glm::mat4 &vp = m_Context->renderer->GetCurrentVP();

    if (m_Context->camera) {
        const Math::Frustum::ViewFrustum frustum = Math::Frustum::FromCameraVP(vp, /*padding=*/Core::HEX_SIZE * 2.0f);
        const Math::Frustum::FrustumPlanes frustum3D = Math::Frustum::FrustumPlanes::FromVP(vp);

        ECS::Systems::MapRenderSystem::RenderAll(m_Registry, *m_Context, frustum);

        ECS::Systems::SpriteBillboardSystem::Update(m_Registry, m_Context->camera, frustum3D);

        ECS::Systems::RenderSystem::Render(m_Registry, *m_Context->renderer, frustum3D);

        ECS::Systems::ParticleSystem::Render(m_Registry, *m_Context->renderer, m_Context->camera);
    }

    if (m_Context->uiSystem) {
        m_Context->uiSystem->Render();
    }
}

void Scenes::Scene::OnExit() {
    std::vector<ECS::EntityID> deadEntities;
    m_Registry.ForEach<ECS::Components::DestroyTagComponent>([&](const ECS::Entity entity, ECS::Components::DestroyTagComponent *) { deadEntities.push_back(static_cast<ECS::EntityID>(entity)); });

    for (const ECS::EntityID id : deadEntities) {
        if (m_Registry.IsValid(id)) {
            m_Registry.DestroyEntity(id);
        }
    }

    ECS::Systems::ScriptSystem::OnSceneExit(m_Registry, *m_Context);
    IO::PrefabManager::ClearCache();
    m_UISystem.Clear();
    m_Context->uiSystem = nullptr;
    m_Context->uiCmdBuf = nullptr;
    LOG_INFO(LOG_WHO, "Exiting Scene " + m_Properties.ScenePath);
}

void Scenes::Scene::OnSaved() { ClearUnsavedChanges(); }
#pragma pop_macro("LOG_WHO")
