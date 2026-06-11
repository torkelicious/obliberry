#include "Renderer.h"
#include "Transform.h"
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

constexpr unsigned int MAX_INSTANCES = 1000000;

Renderer::Renderer() {
    m_InstanceBuffer = std::make_shared<VertexBuffer>(
        nullptr,
        MAX_INSTANCES * sizeof(glm::mat4),
        GL_DYNAMIC_DRAW
    );
}

void Renderer::SetCamera(const Camera &camera) {
    m_Camera = &camera;
    m_VP = camera.GetVP();
}

void Renderer::BeginFrame() {
    m_Commands.clear();
    m_InstancedCommands.clear();
    // todo: serialize glClearColor in some sort of scene properties / metadata
    glClear(GL_COLOR_BUFFER_BIT);
}

// draw elements
void Renderer::Submit(std::shared_ptr<const Mesh> mesh,
                      std::shared_ptr<const Material> material,
                      const Transform &transform) {
    const glm::vec3 &pos = transform.GetPosition();
    int depth = static_cast<int>(std::lround((pos.x + pos.y) * 100.0f));
    int z = static_cast<int>(std::lround(pos.z * 100.0f));
    m_Commands.push_back({mesh, material, transform, depth, z});
}

// instanced calls
void Renderer::Submit(std::shared_ptr<const Mesh> mesh,
                      std::shared_ptr<const Material> material,
                      const std::vector<glm::mat4> *transforms) {
    if (!transforms || transforms->empty()) return;
    m_InstancedCommands.push_back({mesh, material, transforms});
}


void Renderer::Flush() {
    std::ranges::sort(m_Commands, [](const RenderCommand &a, const RenderCommand &b) {
        if (a.sortKeyZ != b.sortKeyZ) return a.sortKeyZ < b.sortKeyZ;
        if (a.sortKeyDepth != b.sortKeyDepth) return a.sortKeyDepth > b.sortKeyDepth;
        if (a.material.get() != b.material.get()) return a.material.get() < b.material.get();
        return a.mesh.get() < b.mesh.get();
    });

    std::shared_ptr<const Material> currentMaterial = nullptr;
    std::shared_ptr<const Mesh> currentMesh = nullptr;

    for (const auto &cmd: m_Commands) {
        if (cmd.material != currentMaterial) {
            cmd.material->shader->Bind();
            cmd.material->shader->SetUniformMat4("u_VP", m_VP);
            const auto *tex =
                    cmd.material->texture ? cmd.material->texture.get() : Texture::White();
            tex->Bind(0);
            cmd.material->shader->SetUniform1i("u_Texture", 0);
            cmd.material->shader->SetUniformVec4("u_Color", cmd.material->color);
            currentMaterial = cmd.material;
        }
        if (cmd.mesh != currentMesh) {
            cmd.mesh->Bind();
            currentMesh = cmd.mesh;
        }
        Execute(cmd);
    }

    m_Commands.clear();
}

void Renderer::InstancedFlush() {
    for (const auto &cmd: m_InstancedCommands) {
        if (cmd.transforms->empty()) continue;

        cmd.material->shader->Bind();
        cmd.material->shader->SetUniformMat4("u_VP", m_VP);
        const auto *tex = cmd.material->texture ? cmd.material->texture.get() : Texture::White();
        tex->Bind(0);
        cmd.material->shader->SetUniform1i("u_Texture", 0);
        cmd.material->shader->SetUniformVec4("u_Color", cmd.material->color);

        size_t instanceCount = cmd.transforms->size();
        if (instanceCount > MAX_INSTANCES) {
            std::cerr << "Exceeded MAX_INSTANCES, Truncating to " << MAX_INSTANCES << ".\n";
            instanceCount = MAX_INSTANCES;
        }
        m_InstanceBuffer->SetSubData(
            cmd.transforms->data(),
            instanceCount * sizeof(glm::mat4)
        );

        cmd.mesh->GetVertexArray()->Bind();
        GLuint vaoID = cmd.mesh->GetVertexArray()->GetID();
        if (m_ConfiguredInstancedVAOs.find(vaoID) == m_ConfiguredInstancedVAOs.end()) {
            cmd.mesh->GetVertexArray()->AddInstancedBuffer(*m_InstanceBuffer, 2);
            m_ConfiguredInstancedVAOs.insert(vaoID);
        } else {
            for (int i = 0; i < 4; i++)
                glEnableVertexAttribArray(2 + i);
        }
        glDrawElementsInstanced(
            GL_TRIANGLES,
            cmd.mesh->GetIndexCount(),
            GL_UNSIGNED_INT,
            nullptr,
            instanceCount
        );
    }
    m_InstancedCommands.clear();
}


void Renderer::Execute(const RenderCommand &cmd) {
    glm::mat4 model = cmd.transform.GetMatrix();
    for (int i = 0; i < 4; i++) {
        glDisableVertexAttribArray(2 + i);
        glVertexAttrib4fv(2 + i, glm::value_ptr(model[i])); // epic troll
    }
    glDrawElements(GL_TRIANGLES, cmd.mesh->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
}


void Renderer::Clean() {
    m_Commands.clear();
    m_InstancedCommands.clear();
}
