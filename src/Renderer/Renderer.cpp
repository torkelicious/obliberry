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
}

void Renderer::Submit(const Mesh &mesh, const Material &material, const Transform &transform) {
    m_Commands.push_back({&mesh, &material, transform});
}

void Renderer::Flush() {
    std::sort(
        m_Commands.begin(),
        m_Commands.end(),
        [](const RenderCommand &a, const RenderCommand &b) {

            // i dont really think this layer sorting is ideal but whatevver it works for now :)

            // primary Z layer
            const int zA = static_cast<int>(std::lround(a.transform.GetPosition().z * 100.0f));
            const int zB = static_cast<int>(std::lround(b.transform.GetPosition().z * 100.0f));
            if (zA != zB)
                return zA < zB;

            // secondary Y depth
            const int yA = static_cast<int>(std::lround(a.transform.GetPosition().y * 100.0f));
            const int yB = static_cast<int>(std::lround(b.transform.GetPosition().y * 100.0f));
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
    glm::mat4 mvp = m_VP * cmd.transform.GetMatrix();

    cmd.material->shader->SetUniformMat4("u_MVP", mvp);
    glDrawElements(GL_TRIANGLES, cmd.mesh->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
}