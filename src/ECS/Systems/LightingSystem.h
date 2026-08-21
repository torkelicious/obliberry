#pragma once
#include "Core/Constants.h"
#include "Core/ResourceManager.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Components/PointLightComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Registry.h"
#include "Rendering/Renderer.h"
#include "Rendering/InternalShaders.h"
#include "Rendering/MeshFactory.h"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
#include <ranges>

namespace ECS::Systems::LightingSystem {

    inline void GenerateLightmap(Components::MapComponent &map, Core::ResourceManager *resources) {
        auto &lm = map.lightmap;

        // compute map bounds from tiles
        glm::vec2 minWorld(std::numeric_limits<float>::max());
        glm::vec2 maxWorld(std::numeric_limits<float>::lowest());

        for (const auto &tile : map.grid.tiles | std::views::values) {
            const glm::vec2 wp = tile.worldPos;
            minWorld = glm::min(minWorld, wp);
            maxWorld = glm::max(maxWorld, wp);
        }

        minWorld -= glm::vec2(Core::HEX_SIZE);
        maxWorld += glm::vec2(Core::HEX_SIZE);

        lm.mapOffset = minWorld;
        lm.mapSize = maxWorld - minWorld;

        const int texW = std::max(1, static_cast<int>(lm.mapSize.x / Core::HEX_SIZE) * Core::LIGHTMAP_TEXELS_PER_HEX);
        const int texH = std::max(1, static_cast<int>(lm.mapSize.y / Core::HEX_SIZE) * Core::LIGHTMAP_TEXELS_PER_HEX);

        // pull the light shader
        auto lightShader = resources ? resources->Get<Rendering::Shader>("[Engine] Light") : nullptr;
        if (!lightShader) {
            // Fallback and create directly
            lightShader = std::make_shared<Rendering::Shader>(Rendering::BuiltinShaders::kLightVert, Rendering::BuiltinShaders::kLightFrag, "<light>");
            Rendering::Renderer::SubmitInitTask(Platform::Threading::SmallTask([lightShader] { lightShader->InitGL(); }));
        }

        // create quad mesh on game thread
        auto lightQuad = std::make_shared<Rendering::Mesh>(Rendering::MeshFactory::CreateQuad());
        auto vao = std::make_shared<Rendering::VertexArray>();
        auto fbo = std::make_shared<Rendering::FrameBuffer>();
        lm.lightQuad = lightQuad;
        lm.lightShader = lightShader;
        lm.lightQuadVAO = vao;
        lm.framebuffer = fbo;
        lm.lastLightCount = std::numeric_limits<size_t>::max();

        Rendering::Renderer::SubmitInitTask([fbo, vao, lightQuad, texW, texH] {
            lightQuad->InitGL();
            vao->Init();
            vao->Bind();
            vao->AddBuffer(lightQuad->GetVBO(), Rendering::VertexTraits<Rendering::Vertex>::GetLayout());
            vao->SetIndexBuffer(lightQuad->GetIBO());
            glBindVertexArray(0);
            fbo->Invalidate(texW, texH);
        });
    }

    inline void Update(Registry &reg) {
        auto *mapComp = reg.GetFirst<Components::MapComponent>();
        if (!mapComp || !mapComp->lightmap.framebuffer) {
            return;
        }

        auto &lm = mapComp->lightmap;
        auto *lightPool = reg.GetPool<Components::PointLightComponent>();
        const size_t lightCount = lightPool->GetDenseEntities().size();

        // capture shared_ptrs
        auto fbo = lm.framebuffer;
        auto shader = lm.lightShader;
        auto quad = lm.lightQuad;
        auto quadVAO = lm.lightQuadVAO;
        const float ambient = lm.ambient;
        const glm::vec2 mapOffset = lm.mapOffset;
        const glm::vec2 mapSize = lm.mapSize;
        const int texW = fbo->GetWidth();
        const int texH = fbo->GetHeight();

        // pack lights; re-render only if the state changed
        std::vector<Rendering::GPULight> packedLights;
        packedLights.reserve(lightCount);

        auto *transformPool = reg.GetPool<Components::TransformComponent>();
        for (const EntityID id : lightPool->GetDenseEntities()) {
            auto *light = lightPool->Get(id);
            const auto *transform = transformPool ? transformPool->Get(id) : nullptr;
            if (!light || !transform || light->intensity <= 0.0f)
                continue;

            const glm::vec3 pos = transform->worldTransform.GetPosition();
            packedLights.push_back({
                    pos.x,
                    pos.y,
                    light->radius,
                    light->color.r * light->intensity,
                    light->color.g * light->intensity,
                    light->color.b * light->intensity,
            });
            light->dirty = false;
        }

        // skip when unchanged
        if (lightCount == lm.lastLightCount && packedLights == lm.lastPackedLights)
            return;

        if (packedLights.empty()) {
            lm.lastLightCount = lightCount;
            lm.lastPackedLights.clear();

            Rendering::Renderer::SubmitInitTask([fbo, ambient] {
                GLint prevFbo;
                glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);

                fbo->Bind();
                constexpr GLenum colorBuf = GL_COLOR_ATTACHMENT0;
                glDrawBuffers(1, &colorBuf);
                glClearColor(ambient, ambient, ambient, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);

                glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);
            });
            return;
        }

        // submit to renderer
        Rendering::Renderer::SubmitInitTask([fbo, shader, quad, quadVAO, lights = packedLights, ambient, mapOffset, mapSize, texW, texH] {
            // Save only the GL state this pass modifies
            GLint prevFbo, prevProgram, prevVao, prevBlendSrc, prevBlendDst, prevBlendEq;
            GLenum prevDrawBuf;
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
            glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
            glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVao);
            const GLboolean prevBlend = glIsEnabled(GL_BLEND);
            glGetIntegerv(GL_BLEND_SRC_RGB, &prevBlendSrc);
            glGetIntegerv(GL_BLEND_DST_RGB, &prevBlendDst);
            glGetIntegerv(GL_BLEND_EQUATION_RGB, &prevBlendEq);
            glGetIntegerv(GL_DRAW_BUFFER0, reinterpret_cast<GLint *>(&prevDrawBuf));
            GLfloat prevClear[4];
            glGetFloatv(GL_COLOR_CLEAR_VALUE, prevClear);

            fbo->Bind();

            // skip entity ID attachment
            constexpr GLenum colorBuf = GL_COLOR_ATTACHMENT0;
            glDrawBuffers(1, &colorBuf);
            // clear to ambient
            glClearColor(ambient, ambient, ambient, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            // additive blending
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            glBlendEquation(GL_FUNC_ADD);
            // bind
            shader->Bind();
            const glm::mat4 proj = glm::ortho(mapOffset.x, mapOffset.x + mapSize.x, mapOffset.y, mapOffset.y + mapSize.y, -1.0f, 1.0f);
            shader->SetUniformMat4("u_projection", proj);
            quadVAO->Bind();

            // one scaled quad per light
            for (const auto &[x, y, radius, colorR, colorG, colorB] : lights) {
                //  translate to world pos, scale unit quad [-0.5,0.5] to [-radius, +radius]
                glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));
                model = glm::scale(model, glm::vec3(2.0f * radius));

                shader->SetUniformMat4("u_model", model);
                shader->SetUniformVec3("u_color", glm::vec3(colorR, colorG, colorB));
                shader->SetUniform1f("u_intensity", 1.0f); // intensity pre multiplied into color
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(quad->GetIndexCount()), GL_UNSIGNED_INT, nullptr);
            }

            // restore state
            glBindVertexArray(prevVao);
            glUseProgram(prevProgram);
            glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);
            glDrawBuffers(1, &prevDrawBuf);
            glClearColor(prevClear[0], prevClear[1], prevClear[2], prevClear[3]);
            prevBlend ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
            glBlendFuncSeparate(prevBlendSrc, prevBlendDst, prevBlendSrc, prevBlendDst);
            glBlendEquationSeparate(prevBlendEq, prevBlendEq);
        });
        lm.lastLightCount = lightCount;
        lm.lastPackedLights = std::move(packedLights);
    }
} // namespace ECS::Systems::LightingSystem
