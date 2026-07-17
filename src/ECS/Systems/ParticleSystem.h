#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>

#include "ECS/Components/ParticleEmitterComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/ECS.h"
#include "Rendering/MeshFactory.h"
#include "Rendering/ParticlePool.h"
#include "Rendering/Renderer.h"

namespace ECS::Systems::ParticleSystem {

    struct EmitterState {
        Rendering::ParticlePool pool;
        EntityID sourceEntity = 0;
    };

    inline std::vector<EmitterState> &GetEmitters() {
        static std::vector<EmitterState> emitters;
        return emitters;
    }

    inline std::shared_ptr<Rendering::Mesh> &GetQuadMesh() {
        static std::shared_ptr<Rendering::Mesh> quad;
        return quad;
    }

    inline void EnsureQuad() {
        auto &q = GetQuadMesh();
        if (!q) {
            q = std::make_shared<Rendering::Mesh>(Rendering::MeshFactory::CreateQuad());
            Rendering::Renderer::SubmitInitTask(Platform::Threading::SmallTask([q] { q->InitGL(); }));
        }
    }

    inline int FindEmitter(EntityID entity) {
        const auto &emitters = GetEmitters();
        for (int i = 0; i < static_cast<int>(emitters.size()); ++i) {
            if (emitters[i].sourceEntity == entity)
                return i;
        }
        return -1;
    }

    inline int CreateEmitter(EntityID entity, const Components::ParticleEmitterComponent &comp) {
        auto &emitters = GetEmitters();
        int index = static_cast<int>(emitters.size());

        Rendering::ParticleEmitterConfig config;
        config.maxParticles = comp.maxParticles;
        config.emitRate = comp.emitRate;
        config.lifetimeMin = comp.lifetimeMin;
        config.lifetimeMax = comp.lifetimeMax;
        config.velocityMin = comp.velocityMin;
        config.velocityMax = comp.velocityMax;
        config.gravity = comp.gravity;
        config.sizeStartMin = comp.sizeStartMin;
        config.sizeStartMax = comp.sizeStartMax;
        config.sizeEndMin = comp.sizeEndMin;
        config.sizeEndMax = comp.sizeEndMax;
        config.rotationSpeedMin = comp.rotationSpeedMin;
        config.rotationSpeedMax = comp.rotationSpeedMax;
        config.colorStart = comp.colorStart;
        config.colorEnd = comp.colorEnd;
        config.isBillboard = comp.isBillboard;
        config.material = comp.material;

        emitters.push_back({Rendering::ParticlePool(std::move(config)), entity});
        return index;
    }

    inline void Update(ECS::Registry &registry, float dt) {
        EnsureQuad();

        auto &emitters = GetEmitters();

        registry.ForEach<Components::ParticleEmitterComponent, Components::TransformComponent>([&](Entity entity, Components::ParticleEmitterComponent *comp, Components::TransformComponent *tc) {
            if (!comp || !comp->active)
                return;

            if (comp->emitterIndex < 0) {
                comp->emitterIndex = FindEmitter(static_cast<EntityID>(entity));
                if (comp->emitterIndex < 0) {
                    comp->emitterIndex = CreateEmitter(static_cast<EntityID>(entity), *comp);
                }
            }

            auto &state = emitters[comp->emitterIndex];

            Rendering::ParticleEmitterConfig config;
            config.maxParticles = comp->maxParticles;
            config.emitRate = comp->emitRate;
            config.lifetimeMin = comp->lifetimeMin;
            config.lifetimeMax = comp->lifetimeMax;
            config.velocityMin = comp->velocityMin;
            config.velocityMax = comp->velocityMax;
            config.gravity = comp->gravity;
            config.sizeStartMin = comp->sizeStartMin;
            config.sizeStartMax = comp->sizeStartMax;
            config.sizeEndMin = comp->sizeEndMin;
            config.sizeEndMax = comp->sizeEndMax;
            config.rotationSpeedMin = comp->rotationSpeedMin;
            config.rotationSpeedMax = comp->rotationSpeedMax;
            config.colorStart = comp->colorStart;
            config.colorEnd = comp->colorEnd;
            config.isBillboard = comp->isBillboard;
            config.material = comp->material;
            state.pool.SetConfig(config);

            if (comp->emitRate > 0.0f) {
                comp->emitAccumulator += dt * comp->emitRate;

                const glm::vec3 origin = tc->transform.GetPosition();

                while (comp->emitAccumulator >= 1.0f && !state.pool.IsFull()) {
                    const float life = glm::linearRand(comp->lifetimeMin, comp->lifetimeMax);

                    const float sizeS = glm::linearRand(comp->sizeStartMin, comp->sizeStartMax);
                    const float sizeE = glm::linearRand(comp->sizeEndMin, comp->sizeEndMax);
                    const float rotSpeed = glm::linearRand(comp->rotationSpeedMin, comp->rotationSpeedMax);

                    state.pool.Spawn(origin, glm::linearRand(comp->velocityMin, comp->velocityMax), comp->gravity, sizeS, sizeE, comp->colorStart, comp->colorEnd, life, rotSpeed);

                    comp->emitAccumulator -= 1.0f;
                }

                if (comp->emitAccumulator > 1.0f)
                    comp->emitAccumulator = 0.0f;
            }

            state.pool.Update(dt);
        });

        for (int i = static_cast<int>(emitters.size()) - 1; i >= 0; --i) {
            if (!registry.IsValid(emitters[i].sourceEntity)) {
                if (i != static_cast<int>(emitters.size()) - 1) {
                    emitters[i] = std::move(emitters.back());
                    if (auto *comp = registry.GetComponent<Components::ParticleEmitterComponent>(emitters[i].sourceEntity)) {
                        comp->emitterIndex = i;
                    }
                }
                emitters.pop_back();
            }
        }
    }

    inline void Render(ECS::Registry &registry, Rendering::Renderer &renderer, const Rendering::Camera *camera = nullptr) {
        const auto &emitters = GetEmitters();
        const auto &quad = GetQuadMesh();
        if (!quad)
            return;

        static std::vector<glm::mat4> transforms;
        static std::vector<glm::vec4> colors;

        const glm::vec3 camRight = camera ? camera->GetRightVector() : glm::vec3(1.0f, 0.0f, 0.0f);
        const glm::vec3 camUp = camera ? camera->GetUpVector() : glm::vec3(0.0f, 1.0f, 0.0f);

        registry.ForEach<Components::ParticleEmitterComponent>([&](Entity entity, const Components::ParticleEmitterComponent *comp) {
            if (!comp || comp->emitterIndex < 0 || comp->emitterIndex >= static_cast<int>(emitters.size()))
                return;

            const auto &state = emitters[comp->emitterIndex];
            if (state.pool.IsEmpty())
                return;

            const bool bb = comp->isBillboard && camera;
            state.pool.BuildTransformsAndColors(transforms, colors, bb, &camRight, &camUp);

            const int blendMode = (comp->blendMode == Components::ParticleBlendMode::Additive) ? 1 : 0;
            renderer.Submit(quad, comp->material.get(), transforms, colors, blendMode, comp->renderOrder);
        });
    }

} // namespace ECS::Systems::ParticleSystem
