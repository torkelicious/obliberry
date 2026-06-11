#include "Renderer.h"
#include "Transform.h"
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

void Renderer::SetCamera(const Camera &camera) {
    m_Camera = &camera;
    m_VP = camera.GetVP();
}

void Renderer::BeginFrame() {
    m_Commands.clear();
    // todo: serialize glClearColor in some sort of scene properties / metadata
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // for 2d
    glDisable(GL_DEPTH_TEST);
}

void Renderer::Submit(std::shared_ptr<const Mesh> mesh,
                      std::shared_ptr<const Material> material,
                      const Transform &transform) {
    const glm::vec3 &pos = transform.GetPosition();
    int depth = static_cast<int>(std::lround((pos.x + pos.y) * 100.0f));
    int z = static_cast<int>(std::lround(pos.z * 100.0f));
    m_Commands.push_back({mesh, material, transform, depth, z});
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
    glEnable(GL_DEPTH_TEST);
}

void Renderer::Execute(const RenderCommand &cmd) {
    cmd.material->shader->SetUniformMat4("u_Model", cmd.transform.GetMatrix());
    glDrawElements(GL_TRIANGLES, cmd.mesh->GetIndexCount(), GL_UNSIGNED_INT,
                   nullptr);
}


void Renderer::Clean() {
    m_Commands.clear();
}
