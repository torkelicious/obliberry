#include "Renderer.h"
#include "Transform.h"
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <ranges>

constexpr unsigned int MAX_INSTANCES = 1000000;

std::vector<std::function<void()> > Renderer::s_InitQueue;
std::mutex Renderer::s_InitQueueMutex;

void Renderer::SetCamera(const Camera &camera, const float aspect) {
    m_Camera = &camera;
    m_VP = camera.GetVP(aspect);
}

void Renderer::BeginFrame() {
    m_Commands.clear();
    m_InstancedCommands.clear();
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::Submit(const Mesh *mesh,
                      const Material *material,
                      const Transform &transform,
                      const Texture *textureOverride) {
    const glm::vec3 &pos = transform.GetPosition();
    const int depth = static_cast<int>(std::lround((pos.x + pos.y) * 100.0f));
    const int z = static_cast<int>(std::lround(pos.z * 100.0f));
    m_Commands.push_back({mesh, material, transform, textureOverride, depth, z});
}

// instanced calls
void Renderer::Submit(const Mesh *mesh,
                      const Material *material,
                      const std::vector<glm::mat4> *transforms,
                      const bool isDirty) {
    if (!transforms || transforms->empty()) return;
    m_InstancedCommands.push_back({mesh, material, transforms, isDirty});
}

void Renderer::Flush() {
    std::ranges::sort(m_Commands, [](const RenderCommand &a, const RenderCommand &b) {
        if (a.sortKeyZ != b.sortKeyZ) return a.sortKeyZ < b.sortKeyZ;
        if (a.sortKeyDepth != b.sortKeyDepth) return a.sortKeyDepth > b.sortKeyDepth;
        if (a.material->shader->GetID() != b.material->shader->GetID())
            return a.material->shader->GetID() < b.material->shader->GetID();

        const Texture *texA = a.textureOverride ? a.textureOverride : a.material->texture.get();
        const Texture *texB = b.textureOverride ? b.textureOverride : b.material->texture.get();

        if (texA && texB)
            return texA->GetPath() < texB->GetPath();

        return texA < texB;
    });


    Shader *currentShader = nullptr;
    const Texture *currentTexture = nullptr;

    for (const auto &cmd: m_Commands) {
        if (!cmd.mesh || !cmd.material || !cmd.material->shader || !cmd.material->shader->IsValid()) continue;

        if (currentShader != cmd.material->shader.get()) {
            currentShader = cmd.material->shader.get();
            currentShader->Bind();
            currentShader->SetUniformMat4("u_VP", m_VP);
            BindLightmap(currentShader);
        }

        if (const Texture *texToUse = cmd.textureOverride ? cmd.textureOverride : cmd.material->texture.get()) {
            if (currentTexture != texToUse) {
                currentTexture = texToUse;
                currentTexture->Bind(0);
                currentShader->SetUniform1i("u_Texture", 0);
            }
        } else {
            // FIX: Ensure the renderer rememebrs that it bound the White texture!
            if (currentTexture != Texture::White()) {
                Texture::White()->Bind(0);
                currentShader->SetUniform1i("u_Texture", 0);
                currentTexture = Texture::White();
            }
        }

        currentShader->SetUniformVec4("u_Color", cmd.material->color);
        cmd.mesh->Bind();
        Execute(cmd);
    }
    m_Commands.clear();
}


void Renderer::InstancedFlush() {
    for (const auto &[mesh, material, transforms, isDirty]: m_InstancedCommands) {
        if (!mesh || !material || !material->shader || !material->shader->IsValid()) continue;
        const auto instanceCount = static_cast<unsigned int>(transforms->size());
        if (instanceCount == 0 || instanceCount > MAX_INSTANCES) continue;

        material->shader->Bind();
        material->shader->SetUniformMat4("u_VP", m_VP);
        BindLightmap(material->shader.get());

        if (material->texture) {
            material->texture->Bind(0);
            material->shader->SetUniform1i("u_Texture", 0);
        } else {
            Texture::White()->Bind(0);
            material->shader->SetUniform1i("u_Texture", 0);
        }

        material->shader->SetUniformVec4("u_Color", material->color);

        std::shared_ptr<VertexBuffer> vbo;
        std::shared_ptr<VertexArray> vao;

        auto it = m_InstanceGroups.find(transforms);

        if (it == m_InstanceGroups.end()) {
            vbo = std::make_shared<VertexBuffer>();
            vbo->Init(transforms->data(), instanceCount * sizeof(glm::mat4), GL_DYNAMIC_DRAW);

            vao = std::make_shared<VertexArray>();
            vao->Init();

            vao->Bind();
            vao->AddBuffer(mesh->GetVBO(), VertexTraits<Vertex>::GetLayout());
            vao->SetIndexBuffer(mesh->GetIBO());
            vao->AddInstancedBuffer(*vbo, 2);

            m_InstanceGroups[transforms] = {vbo, vao};
        } else {
            vbo = it->second.vbo;
            vao = it->second.vao;
        }

        if (isDirty && it != m_InstanceGroups.end()) {
            vbo->SetSubData(transforms->data(), instanceCount * sizeof(glm::mat4));
        }

        vao->Bind();
        glDrawElementsInstanced(
            GL_TRIANGLES,
            mesh->GetIndexCount(),
            GL_UNSIGNED_INT,
            nullptr,
            instanceCount
        );
    }
    m_InstancedCommands.clear();
}


void Renderer::Clean() {
    m_Commands.clear();
    m_InstancedCommands.clear();
}

void Renderer::SetLightmap(const Lightmap *lightmap) {
    m_Lightmap = lightmap;
}

void Renderer::BindLightmap(Shader *shader) const {
    if (m_Lightmap && m_Lightmap->texture) {
        m_Lightmap->texture->Bind(1);
        shader->SetUniform1i("u_LightTexture", 1);
        shader->SetUniformVec2("u_MapSize", m_Lightmap->mapSize);
        shader->SetUniformVec2("u_MapOffset", m_Lightmap->mapOffset);
        shader->SetUniform1f("u_Ambient", m_Lightmap->ambient);
    }
}

void Renderer::SetClearColor(const glm::vec4 color) {
    glClearColor(color[0], color[1], color[2], color[3]);
}

void Renderer::Execute(const RenderCommand &cmd) {
    glm::mat4 model = cmd.transform.GetMatrix();
    for (int i = 0; i < 4; i++) {
        glDisableVertexAttribArray(2 + i);
        glVertexAttrib4fv(2 + i, glm::value_ptr(model[i]));
    }
    glDrawElements(GL_TRIANGLES, cmd.mesh->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
}

void Renderer::SubmitInitTask(std::function<void()> task) {
    std::lock_guard lock(s_InitQueueMutex);
    s_InitQueue.push_back(std::move(task));
}

void Renderer::ProcessInitQ() {
    std::vector<std::function<void()> > queueCopy;
    {
        std::lock_guard lock(s_InitQueueMutex);
        queueCopy = std::move(s_InitQueue);
    }
    for (auto &task: queueCopy) {
        task();
    }
}
