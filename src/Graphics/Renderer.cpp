#include "Renderer.h"
#include <algorithm>
#include <cassert>
#include "Camera.h"

void Renderer::BeginFrame(const Camera &camera) {
    m_Camera = &camera;
    m_Commands.clear();
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::Submit(
    const Mesh &mesh,
    const Material &material,
    const Transform &transform) {
    m_Commands.push_back({
        &mesh,
        &material,
        transform
    });
}

void Renderer::Flush() {
    assert(m_Camera != nullptr);

    std::sort(
        m_Commands.begin(),
        m_Commands.end(),
        [](const RenderCommand &a, const RenderCommand &b) {
            if (a.material != b.material)
                return a.material < b.material;
            return a.mesh < b.mesh;
        });

    const Material *currentMaterial = nullptr;
    const Mesh *currentMesh = nullptr;

    for (const auto &cmd: m_Commands) {
        if (cmd.material != currentMaterial) {
            cmd.material->shader->Bind();

            auto *tex = cmd.material->texture ? cmd.material->texture : Texture::White();
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

void Renderer::Execute(const RenderCommand &cmd) {
    const glm::mat4 mvp =
            m_Camera->GetVP() *
            TransformToMatrix(cmd.transform);

    cmd.material->shader->SetUniformMat4("u_MVP", mvp);

    glDrawElements(
        GL_TRIANGLES,
        cmd.mesh->GetIndexCount(),
        GL_UNSIGNED_INT,
        nullptr
    );
}
