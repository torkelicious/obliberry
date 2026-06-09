#include "Renderer.h"
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include "Transform.h"

void Renderer::SetCamera(const Camera &camera, float width, float height) {
    m_Camera = &camera;
    m_VP = camera.GetVP();
}

void Renderer::BeginFrame() {
    m_Commands.clear();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // for 2d
    glDisable(GL_DEPTH_TEST);
}

void Renderer::Submit(const Mesh &mesh, const Material &material, const Transform &transform) {
    m_Commands.push_back({&mesh, &material, transform});
}

void Renderer::Flush() {
    std::sort(
        m_Commands.begin(),
        m_Commands.end(),
        [](const RenderCommand &a, const RenderCommand &b) {
            const glm::vec3 &posA = a.transform.GetPosition();
            const glm::vec3 &posB = b.transform.GetPosition();
            // sort by isometric depth (x + y)
            const float keyA = posA.x + posA.y;
            const float keyB = posB.x + posB.y;
            // to integers
            const int depthA = static_cast<int>(std::lround(keyA * 100.0f));
            const int depthB = static_cast<int>(std::lround(keyB * 100.0f));
            if (depthA != depthB) {
                return depthA > depthB;
            }

            // height / layer layering (z)
            // use world z as a tie-breaker.
            const int zA = static_cast<int>(std::lround(posA.z * 100.0f));
            const int zB = static_cast<int>(std::lround(posB.z * 100.0f));
            if (zA != zB) {
                return zA < zB;
            }
            // material batching fallback
            if (a.material != b.material)
                return a.material < b.material;

            // mesh batching fallback
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
            const auto *tex = material->texture ? material->texture.get() : Texture::White();
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
    glEnable(GL_DEPTH_TEST);
}

void Renderer::Execute(const RenderCommand &cmd) {
    cmd.material->shader->SetUniformMat4("u_Model", cmd.transform.GetMatrix());
    glDrawElements(GL_TRIANGLES, cmd.mesh->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
}
