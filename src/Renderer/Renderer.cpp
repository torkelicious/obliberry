#include "Renderer.h"
#include "Transform.h"
#include <algorithm>
#include <cstdint>
#include <glm/gtc/type_ptr.hpp>
#include <memory>

constexpr unsigned int MAX_INSTANCES = 100000;
// for lazy assign l8r
constexpr size_t INSTANCE_BUFFER_SIZE = MAX_INSTANCES * sizeof(glm::mat4);

std::vector<std::function<void()>> Renderer::s_InitQueue;
std::mutex Renderer::s_InitQueueMutex;

static glm::vec4 s_ClearColorStaging = {0.0f, 0.0f, 0.0f, 1.0f};

void Renderer::SetCamera(const Camera& camera, const float aspect) {
    m_Camera = &camera;
    m_Aspect = aspect;
}

void Renderer::BeginFrame() {
    m_Commands[m_SubmitIndex].clear();
    m_InstancedCommands[m_SubmitIndex].clear();
    m_Lightmap[m_SubmitIndex] = nullptr;

    if (m_Camera) {
        m_VP[m_SubmitIndex] = m_Camera->GetVP(m_Aspect);
    }
}

void Renderer::Submit(const std::shared_ptr<Mesh>& mesh, const Material* material, const Transform& transform,
                      const Texture* textureOverride) {
    const glm::vec3& pos = transform.GetPosition();

    auto packKey = [](const float posX, const float posY, const float posZ) -> int32_t {
        int d = static_cast<int>(std::lround((posX + posY) * 100.0f));
        int z = static_cast<int>(std::lround(posZ * 100.0f));
        // signed 16-bit range
        d = std::clamp(d, -32767, 32767);
        z = std::clamp(z, -32767, 32767);
        return static_cast<int32_t>(static_cast<uint32_t>(static_cast<int16_t>(d)) << 16 |
                                    static_cast<uint32_t>(static_cast<int16_t>(z)));
    };

    const Texture* effectiveTex = textureOverride                 ? textureOverride
                                  : material && material->texture ? material->texture.get()
                                                                  : nullptr;
    const glm::vec4 col = material ? material->color : glm::vec4(1.0f);

    m_Commands[m_SubmitIndex].push_back(
        {mesh.get(), material, effectiveTex, col, transform.GetMatrix(), packKey(pos.x, pos.y, pos.z)});
}

void Renderer::Submit(const std::shared_ptr<Mesh>& mesh, const Material* material,
                      const std::vector<glm::mat4>& transforms) {
    if (transforms.empty())
        return;

    const Texture* tex = material && material->texture ? material->texture.get() : nullptr;
    const glm::vec4 col = material ? material->color : glm::vec4(1.0f);

    m_InstancedCommands[m_SubmitIndex].push_back({mesh.get(), material, tex, col, transforms});
}

void Renderer::Flush(const size_t renderIndex) {
    if (!m_Commands[renderIndex].empty()) {
        std::ranges::sort(m_Commands[renderIndex], [](const RenderCommand& a, const RenderCommand& b) {
            const auto depthA = static_cast<int16_t>(a.sortKey >> 16);
            const auto depthB = static_cast<int16_t>(b.sortKey >> 16);
            const auto zA = static_cast<int16_t>(a.sortKey & 0xFFFF);
            if (const auto zB = static_cast<int16_t>(b.sortKey & 0xFFFF); zA != zB)
                return zA < zB;
            if (depthA != depthB)
                return depthA > depthB;
            if (a.material != b.material)
                return a.material < b.material;
            if (a.effectiveTexture != b.effectiveTexture)
                return a.effectiveTexture < b.effectiveTexture;
            if (a.mesh != b.mesh)
                return a.mesh < b.mesh;
            return false;
        });
    }

    struct Batch {
        BatchKey key;
        std::vector<glm::mat4> transforms;
    };

    std::vector<Batch> batches;
    batches.reserve(m_InstancedCommands[renderIndex].size() + m_Commands[renderIndex].size());

    // merge instanced commands by sorting first
    {
        struct InstancedEntry {
            BatchKey key;
            const std::vector<glm::mat4>* transforms;
        };

        std::vector<InstancedEntry> entries;
        entries.reserve(m_InstancedCommands[renderIndex].size());
        for (const auto& [mesh, material, effectiveTexture, color, transforms] : m_InstancedCommands[renderIndex]) {
            if (!mesh || !material || !material->shader || !material->shader->IsValid())
                continue;
            if (transforms.empty())
                continue;
            entries.push_back({{mesh, material, effectiveTexture, color}, &transforms});
        }

        // sort by BatchKey
        std::ranges::sort(entries, [](const InstancedEntry& a, const InstancedEntry& b) {
            if (a.key.mesh != b.key.mesh)
                return a.key.mesh < b.key.mesh;
            if (a.key.material != b.key.material)
                return a.key.material < b.key.material;
            if (a.key.texture != b.key.texture)
                return a.key.texture < b.key.texture;
            if (a.key.color.r != b.key.color.r)
                return a.key.color.r < b.key.color.r;
            if (a.key.color.g != b.key.color.g)
                return a.key.color.g < b.key.color.g;
            if (a.key.color.b != b.key.color.b)
                return a.key.color.b < b.key.color.b;
            return a.key.color.a < b.key.color.a;
        });

        // merge
        for (size_t i = 0; i < entries.size();) {
            Batch b;
            b.key = entries[i].key;
            size_t total = entries[i].transforms->size();
            size_t j = i + 1;
            while (j < entries.size() && entries[j].key == entries[i].key) {
                total += entries[j].transforms->size();
                ++j;
            }
            b.transforms.reserve(total);
            for (size_t k = i; k < j; ++k) {
                const auto& src = *entries[k].transforms;
                b.transforms.insert(b.transforms.end(), src.begin(), src.end());
            }
            batches.push_back(std::move(b));
            i = j;
        }
    }

    // already sorted just merge.
    BatchKey currentKey{};
    std::vector<glm::mat4>* currentMats = nullptr;
    m_LastBoundVAO = nullptr;
    for (const auto& cmd : m_Commands[renderIndex]) {
        if (!cmd.mesh || !cmd.material || !cmd.material->shader || !cmd.material->shader->IsValid())
            continue;
        if (BatchKey key{cmd.mesh, cmd.material, cmd.effectiveTexture, cmd.color}; !currentMats || currentKey != key) {
            batches.push_back({key, {}});
            currentKey = key;
            currentMats = &batches.back().transforms;
        }
        currentMats->push_back(cmd.model);
    }

    m_LastBoundVAO = nullptr;
    m_LastBoundShader = nullptr;
    for (const auto& [key, transforms] : batches) {
        RenderBatch(key, transforms, renderIndex);
    }
    m_LastBoundVAO = nullptr;
    m_LastBoundShader = nullptr;

    m_Commands[renderIndex].clear();
    m_InstancedCommands[renderIndex].clear();
}

void Renderer::RenderBatch(const BatchKey& key, const std::vector<glm::mat4>& transforms, const size_t renderIndex) {
    const auto instanceCount = static_cast<unsigned int>(transforms.size());
    if (instanceCount == 0 || instanceCount > MAX_INSTANCES)
        return;

    Shader* shader = key.material->shader.get();
    if (!shader || !shader->IsValid())
        return;

    if (m_LastBoundShader != shader) {
        shader->Bind();
        shader->SetUniformMat4("u_VP", m_VP[renderIndex]);
        BindLightmap(shader, renderIndex);
        m_LastBoundShader = shader;
    }

    if (key.texture) {
        key.texture->Bind(0);
        shader->SetUniform1i("u_Texture", 0);
    } else {
        Texture::White()->Bind(0);
        shader->SetUniform1i("u_Texture", 0);
    }

    shader->SetUniformVec4("u_Color", key.color);

    // get / create VAO for this mesh
    auto [vao_it, inserted] = m_MeshVAOs.try_emplace(key.mesh);
    auto& [mesh_vao, instanceAttribReady] = vao_it->second;
    if (inserted) {
        mesh_vao = std::make_shared<VertexArray>();
        mesh_vao->Init();
        mesh_vao->Bind();
        mesh_vao->AddBuffer(key.mesh->GetVBO(), VertexTraits<Vertex>::GetLayout());
        mesh_vao->SetIndexBuffer(key.mesh->GetIBO());
        glBindVertexArray(0);
    }
    const VertexArray* vao = mesh_vao.get();

    // create the instance buffer
    if (!m_DynamicInstanceBuffer) {
        m_DynamicInstanceBuffer = std::make_unique<VertexBuffer>();
        m_DynamicInstanceBuffer->Init(nullptr, INSTANCE_BUFFER_SIZE, GL_DYNAMIC_DRAW);
    }

    m_DynamicInstanceBuffer->SetDataOrphaned(transforms.data(), instanceCount * sizeof(glm::mat4));

    if (m_LastBoundVAO != vao) {
        vao->Bind();
        m_LastBoundVAO = vao;
    }

    if (!instanceAttribReady) {
        vao->AddInstancedBuffer(*m_DynamicInstanceBuffer, 2);
        instanceAttribReady = true;
    }

    glDrawElementsInstanced(GL_TRIANGLES, key.mesh->GetIndexCount(), GL_UNSIGNED_INT, nullptr, instanceCount);
}

void Renderer::Clean() {
    m_Commands[0].clear();
    m_Commands[1].clear();
    m_InstancedCommands[0].clear();
    m_InstancedCommands[1].clear();

    m_MeshVAOs.clear();
    m_LastBoundVAO = nullptr;
    m_LastBoundShader = nullptr;
}

void Renderer::SetLightmap(const Lightmap* lightmap) { m_Lightmap[m_SubmitIndex] = lightmap; }

void Renderer::BindLightmap(Shader* shader, const size_t renderIndex) const {
    if (const Lightmap* lm = m_Lightmap[renderIndex]; lm && lm->texture) {
        lm->texture->Bind(1);
        shader->SetUniform1i("u_LightTexture", 1);
        shader->SetUniformVec2("u_MapSize", lm->mapSize);
        shader->SetUniformVec2("u_MapOffset", lm->mapOffset);
        shader->SetUniform1f("u_Ambient", lm->ambient);
    } else {
        Texture::White()->Bind(1);
        shader->SetUniform1i("u_LightTexture", 1);
    }
}

void Renderer::SetClearColor(const glm::vec4 color) { s_ClearColorStaging = color; }

void Renderer::ApplyClearColor() {
    glClearColor(s_ClearColorStaging[0], s_ClearColorStaging[1], s_ClearColorStaging[2], s_ClearColorStaging[3]);
}

void Renderer::SwapBuffers() {
    m_RenderIndex = m_SubmitIndex;
    m_SubmitIndex = (m_SubmitIndex + 1) % 2;
}

void Renderer::SubmitInitTask(std::function<void()> task) {
    std::lock_guard lock(s_InitQueueMutex);
    s_InitQueue.push_back(std::move(task));
}

void Renderer::ProcessInitQ() {
    std::vector<std::function<void()>> queueCopy;
    {
        std::lock_guard lock(s_InitQueueMutex);
        queueCopy = std::move(s_InitQueue);
    }
    for (auto& task : queueCopy) {
        task();
    }
}

void Renderer::EnsureFramebufferSize(uint32_t width, uint32_t height) {
    if (m_FboWidth == width && m_FboHeight == height)
        return;
    m_FboWidth = width;
    m_FboHeight = height;
    SubmitInitTask([this, width, height]() {
        if (!m_EditorFramebuffer) {
            m_EditorFramebuffer = std::make_shared<FrameBuffer>(width, height);
        } else {
            m_EditorFramebuffer->Invalidate(width, height);
        }
    });
}
