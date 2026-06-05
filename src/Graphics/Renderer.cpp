#include "Renderer.h"
#include <algorithm>
#include <cassert>
#include "Camera.h"

void Renderer::BeginFrame(const Camera &camera) {
    m_Camera = &camera;
    m_Commands.clear();
    //glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::Submit(
    const Mesh &mesh,
    const Material &material,
    const Transform &transform) {
    m_Commands.emplace_back(&mesh, &material, transform);
}

void Renderer::Flush() {
    assert(m_Camera != nullptr);

    std::sort(
        m_Commands.begin(),
        m_Commands.end(),
        [](const RenderCommand &a, const RenderCommand &b) {
            if (std::get < 1 > (a) != std::get < 1 > (b))
                return std::get < 1 > (a) < std::get < 1 > (b);
            return std::get < 0 > (a) < std::get < 0 > (b);
        });

    const Material *currentMaterial = nullptr;
    const Mesh *currentMesh = nullptr;

    for (const auto &cmd: m_Commands) {
        const auto *mesh = std::get < 0 > (cmd);
        const auto *material = std::get < 1 > (cmd);
        const auto &transform = std::get < 2 > (cmd);

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
    const auto *mesh = std::get < 0 > (cmd);
    const auto *material = std::get < 1 > (cmd);
    const auto &transform = std::get < 2 > (cmd);

    const glm::mat4 mvp =
            m_Camera->GetVP() *
            TransformToMatrix(transform);

    material->shader->SetUniformMat4("u_MVP", mvp);

    glDrawElements(
        GL_TRIANGLES,
        mesh->GetIndexCount(),
        GL_UNSIGNED_INT,
        nullptr
    );
}
