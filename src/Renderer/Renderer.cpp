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
    m_DrawCallCount = 0;
    m_Commands.clear();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // for 2d
    glDisable(GL_DEPTH_TEST);
}

void Renderer::Submit(const Mesh &mesh, const Material &material,
                      const Transform &transform) {
    const glm::vec3 &pos = transform.GetPosition();
    int depth = static_cast<int>(std::lround((pos.x + pos.y) * 100.0f));
    int z = static_cast<int>(std::lround(pos.z * 100.0f));
    m_Commands.push_back({&mesh, &material, transform, depth, z});
}

void Renderer::Flush() {
    std::ranges::sort(m_Commands, [](const RenderCommand &a, const RenderCommand &b) {
        if (a.sortKeyZ != b.sortKeyZ) return a.sortKeyZ < b.sortKeyZ;
        if (a.sortKeyDepth != b.sortKeyDepth) return a.sortKeyDepth > b.sortKeyDepth;
        if (a.material != b.material) return a.material < b.material;
        return a.mesh < b.mesh;
    });

    const Material *currentMaterial = nullptr;
    const Mesh *currentMesh = nullptr;

    for (const auto &cmd: m_Commands) {
        const auto *mesh = cmd.mesh;
        const auto *material = cmd.material;
        if (material != currentMaterial) {
            material->shader->Bind();
            // view projection matrix once per shader switch rather than per object
            material->shader->SetUniformMat4("u_VP", m_VP);
            const auto *tex =
                    material->texture ? material->texture.get() : Texture::White();
            tex->Bind(0);
            material->shader->SetUniform1i("u_Texture", 0);
            material->shader->SetUniformVec4("u_Color", material->color);
            currentMaterial = material;
        }

        if (mesh != currentMesh) {
            mesh->Bind();
            currentMesh = mesh;
        }

        Execute(cmd);
    }

    m_Commands.clear();
    m_LastDrawCallCount = m_DrawCallCount;
    glEnable(GL_DEPTH_TEST);
}

void Renderer::Execute(const RenderCommand &cmd) {
    cmd.material->shader->SetUniformMat4("u_Model", cmd.transform.GetMatrix());
    glDrawElements(GL_TRIANGLES, cmd.mesh->GetIndexCount(), GL_UNSIGNED_INT,
                   nullptr);
    ++m_DrawCallCount;
}
