#include "Renderer.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include "Camera.h"

void Renderer::BeginFrame(const Camera &camera) {
    m_Camera = &camera;
    m_VPMatrix = camera.GetProjection() * camera.GetView();
    m_Commands.clear();
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::Submit(
    const Mesh &mesh,
    const Material &material,
    const Transform &transform) {
    m_Commands.push_back({&mesh, &material, transform});
}

void Renderer::Flush() {
    assert(m_Camera != nullptr);

    std::sort(
        m_Commands.begin(),
        m_Commands.end(),
        [](const RenderCommand &a, const RenderCommand &b) {
            // Primary Z-layer
            const int zA = static_cast<int>(std::lround(a.transform.Position.z * 100.0f));
            const int zB = static_cast<int>(std::lround(b.transform.Position.z * 100.0f));
            if (zA != zB)
                return zA < zB;

            // Secondary Y depth
            const int yA = static_cast<int>(std::lround(a.transform.Position.y * 100.0f));
            const int yB = static_cast<int>(std::lround(b.transform.Position.y * 100.0f));
            if (yA != yB)
                return yA > yB;

            //  material batching
            if (a.material != b.material)
                return a.material < b.material;

            //  mesh batching
            return a.mesh < b.mesh;
        });

    const Material *currentMaterial = nullptr;
    const Mesh *currentMesh = nullptr;

    for (const auto &cmd: m_Commands) {
        const auto *mesh = cmd.mesh;
        const auto *material = cmd.material;
        const auto &transform = cmd.transform;

        if (material != currentMaterial) {
            material->shader->Bind();

            auto *tex = material->texture ? material->texture : Texture::White();
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
}

void Renderer::Execute(const RenderCommand &cmd) {
    const glm::mat4 mvp = m_VPMatrix * TransformToMatrix(cmd.transform);
    cmd.material->shader->SetUniformMat4("u_MVP", mvp);

    glDrawElements(
        GL_TRIANGLES,
        cmd.mesh->GetIndexCount(),
        GL_UNSIGNED_INT,
        nullptr
    );
}
