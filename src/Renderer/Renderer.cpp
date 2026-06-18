#include "Renderer.h"
#include "Transform.h"
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

constexpr unsigned int MAX_INSTANCES = 1000000;

void Renderer::SetCamera(const Camera &camera) {
    m_Camera = &camera;
    m_VP = camera.GetVP();
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
        if (a.material != b.material) return a.material < b.material;
        if (a.textureOverride != b.textureOverride) return a.textureOverride < b.textureOverride;
        return a.mesh < b.mesh;
    });

    const Material *currentMaterial = nullptr;
    const Texture *currentTexture = nullptr;
    const Mesh *currentMesh = nullptr;

    for (const auto &cmd: m_Commands) {
        if (cmd.material != currentMaterial) {
            cmd.material->shader->Bind();
            cmd.material->shader->SetUniformMat4("u_VP", m_VP);
            cmd.material->shader->SetUniformVec4("u_Color", cmd.material->color);
            BindLightmap(cmd.material->shader.get());
            currentMaterial = cmd.material;
            currentTexture = nullptr;
        }

        const Texture *texToBind = cmd.textureOverride
                                       ? cmd.textureOverride
                                       : cmd.material->texture
                                             ? cmd.material->texture.get()
                                             : Texture::White();

        if (texToBind != currentTexture) {
            texToBind->Bind(0);
            cmd.material->shader->SetUniform1i("u_Texture", 0);
            currentTexture = texToBind;
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
    std::ranges::sort(m_InstancedCommands, [](const InstancedRenderCommand &a, const InstancedRenderCommand &b) {
        if (a.material != b.material) return a.material < b.material;
        return a.mesh < b.mesh;
    });
    const Material *currentMaterial = nullptr;

    for (const auto &[mesh, material, transforms, isDirty]: m_InstancedCommands) {
        if (transforms->empty()) { continue; }
        if (material != currentMaterial) {
            material->shader->Bind();
            material->shader->SetUniformMat4("u_VP", m_VP);
            const auto *tex = material->texture ? material->texture.get() : Texture::White();
            tex->Bind(0);
            material->shader->SetUniform1i("u_Texture", 0);
            material->shader->SetUniformVec4("u_Color", material->color);
            BindLightmap(material->shader.get());
            currentMaterial = material;
        }

        size_t instanceCount = transforms->size();
        if (instanceCount > MAX_INSTANCES) {
            std::cerr << "Exceeded MAX_INSTANCES Truncating to " << MAX_INSTANCES << ".\n";
            instanceCount = MAX_INSTANCES;
        }

        auto &[vbo, vao] = m_InstanceGroups[transforms];
        if (!vbo) {
            vbo = std::make_shared<VertexBuffer>(nullptr, MAX_INSTANCES * sizeof(glm::mat4), GL_DYNAMIC_DRAW);
            vbo->SetSubData(transforms->data(), instanceCount * sizeof(glm::mat4));

            vao = std::make_shared<VertexArray>();
            vao->AddBuffer(mesh->GetVBO(), VertexTraits<Vertex>::GetLayout());
            vao->SetIndexBuffer(mesh->GetIBO());
            vao->AddInstancedBuffer(*vbo, 2);
        } else if (isDirty) {
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
    if (m_Lightmap &&m_Lightmap



    ->
    texture
    )
    {
        m_Lightmap->texture->Bind(1);
        shader->SetUniform1i("u_LightTexture", 1);
        shader->SetUniformVec2("u_MapSize", m_Lightmap->mapSize);
        shader->SetUniformVec2("u_MapOffset", m_Lightmap->mapOffset);
        shader->SetUniform1f("u_Ambient", m_Lightmap->ambient);
    }
}

void Renderer::SetClearColor(const glm::vec4 color) const {
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
