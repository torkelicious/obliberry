#include "Renderer.h"
#include "Transform.h"
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <memory>

constexpr unsigned int MAX_INSTANCES = 100000;
constexpr size_t INSTANCE_BUFFER_SIZE = MAX_INSTANCES * sizeof(glm::mat4);
constexpr size_t COLOR_BUFFER_SIZE = MAX_INSTANCES * sizeof(glm::vec4);
constexpr size_t ID_BUFFER_SIZE = MAX_INSTANCES * sizeof(int);

using InitTask = std::variant<Platform::Threading::SmallTask, std::function<void()>>;
std::vector<InitTask> Rendering::Renderer::s_InitQueue;
std::mutex Rendering::Renderer::s_InitQueueMutex;
std::atomic<bool> Rendering::Renderer::s_HasInitTasks{false};

static glm::vec4 s_ClearColorStaging = {0.0f, 0.0f, 0.0f, 1.0f};

void Rendering::Renderer::SetCamera(const Camera &camera, const float aspect) {
    m_Camera = &camera;
    m_Aspect = aspect;
}

void Rendering::Renderer::BeginFrame() {
    m_Commands[m_SubmitIndex].clear();
    m_InstancedCommands[m_SubmitIndex].clear();
    m_InstancedTransformsStaging[m_SubmitIndex].clear();
    m_InstancedEntityIDsStaging[m_SubmitIndex].clear();
    m_InstancedColorsStaging[m_SubmitIndex].clear();
    m_Lightmap[m_SubmitIndex] = nullptr;

    if (m_Camera) {
        m_VP[m_SubmitIndex] = m_Camera->GetVP(m_Aspect);
    }
}

void Rendering::Renderer::Submit(const std::shared_ptr<Mesh> &mesh, const Material *material, const Transform &transform, const Texture *textureOverride, const int entityID) {
    const glm::vec3 &pos = transform.GetPosition();

    auto packKey = [](const float posX, const float posY, const float posZ) -> int32_t {
        int d = static_cast<int>(std::lround((posX + posY) * 100.0f));
        int z = static_cast<int>(std::lround(posZ * 100.0f));
        d = std::clamp(d, -32767, 32767);
        z = std::clamp(z, -32767, 32767);
        return static_cast<int32_t>(static_cast<uint32_t>(static_cast<int16_t>(d)) << 16 | static_cast<uint32_t>(static_cast<int16_t>(z)));
    };

    const Texture *effectiveTex = textureOverride ? textureOverride : material && material->texture ? material->texture.get() : nullptr;
    const glm::vec4 col = material ? material->color : glm::vec4(1.0f);

    m_Commands[m_SubmitIndex].push_back({mesh.get(), material, effectiveTex, col, transform.GetMatrix(), packKey(pos.x, pos.y, pos.z), entityID});
}

void Rendering::Renderer::Submit(const std::shared_ptr<Mesh> &mesh, const Material *material, const std::vector<glm::mat4> &transforms, const std::vector<int> &entityIDs) {
    if (transforms.empty())
        return;

    const Texture *tex = material && material->texture ? material->texture.get() : nullptr;
    const glm::vec4 col = material ? material->color : glm::vec4(1.0f);

    const size_t transformOffset = m_InstancedTransformsStaging[m_SubmitIndex].size();
    m_InstancedTransformsStaging[m_SubmitIndex].insert(m_InstancedTransformsStaging[m_SubmitIndex].end(), transforms.begin(), transforms.end());

    const size_t entityIDOffset = m_InstancedEntityIDsStaging[m_SubmitIndex].size();
    m_InstancedEntityIDsStaging[m_SubmitIndex].insert(m_InstancedEntityIDsStaging[m_SubmitIndex].end(), entityIDs.begin(), entityIDs.end());

    m_InstancedCommands[m_SubmitIndex].push_back({.mesh = mesh.get(),
                                                  .material = material,
                                                  .effectiveTexture = tex,
                                                  .color = col,
                                                  .transformPtr = nullptr,
                                                  .transformOffset = transformOffset,
                                                  .transformCount = transforms.size(),
                                                  .entityIDPtr = nullptr,
                                                  .entityIDOffset = entityIDOffset,
                                                  .entityIDCount = entityIDs.size()});
}

void Rendering::Renderer::Submit(const std::shared_ptr<Mesh> &mesh, const Material *material, const std::vector<glm::mat4> &transforms, const std::vector<glm::vec4> &colors) {
    if (transforms.empty())
        return;

    const Texture *tex = material && material->texture ? material->texture.get() : nullptr;
    const glm::vec4 col = material ? material->color : glm::vec4(1.0f);

    const size_t transformOffset = m_InstancedTransformsStaging[m_SubmitIndex].size();
    m_InstancedTransformsStaging[m_SubmitIndex].insert(m_InstancedTransformsStaging[m_SubmitIndex].end(), transforms.begin(), transforms.end());

    const size_t colorOffset = m_InstancedColorsStaging[m_SubmitIndex].size();
    m_InstancedColorsStaging[m_SubmitIndex].insert(m_InstancedColorsStaging[m_SubmitIndex].end(), colors.begin(), colors.end());

    m_InstancedCommands[m_SubmitIndex].push_back({.mesh = mesh.get(),
                                                  .material = material,
                                                  .effectiveTexture = tex,
                                                  .color = col,
                                                  .transformPtr = nullptr,
                                                  .transformOffset = transformOffset,
                                                  .transformCount = transforms.size(),
                                                  .colorPtr = nullptr,
                                                  .colorOffset = colorOffset,
                                                  .colorCount = colors.size(),
                                                  .entityIDPtr = nullptr,
                                                  .entityIDOffset = 0,
                                                  .entityIDCount = 0});
}

void Rendering::Renderer::SubmitPersistent(const std::shared_ptr<Mesh> &mesh, const Material *material, const std::vector<glm::mat4> *transforms, const std::vector<int> *entityIDs) {
    if (!transforms || transforms->empty())
        return;

    const Texture *tex = material && material->texture ? material->texture.get() : nullptr;
    const glm::vec4 col = material ? material->color : glm::vec4(1.0f);

    m_InstancedCommands[m_SubmitIndex].push_back({.mesh = mesh.get(),
                                                  .material = material,
                                                  .effectiveTexture = tex,
                                                  .color = col,
                                                  .transformPtr = transforms->data(),
                                                  .transformOffset = 0,
                                                  .transformCount = transforms->size(),
                                                  .colorPtr = nullptr,
                                                  .colorOffset = 0,
                                                  .colorCount = 0,
                                                  .entityIDPtr = entityIDs && !entityIDs->empty() ? entityIDs->data() : nullptr,
                                                  .entityIDOffset = 0,
                                                  .entityIDCount = entityIDs ? entityIDs->size() : 0});
}

void Rendering::Renderer::Flush(const size_t renderIndex) {
    m_LastBoundVAO = nullptr;
    m_LastBoundShader = nullptr;
    m_LastBoundTexture = nullptr;
    m_LastBoundColor = glm::vec4(0.0f);

    // sort single commands by (z, depth, material, texture, mesh)
    if (m_Commands[renderIndex].size() > 1) {
        std::ranges::sort(m_Commands[renderIndex], [](const RenderCommand &a, const RenderCommand &b) {
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

    // merge and render instanced commands
    {
        if (auto &instCmds = m_InstancedCommands[renderIndex]; !instCmds.empty()) {
            if (instCmds.size() > 1) {
                std::ranges::sort(instCmds, [](const InstancedRenderCommand &a, const InstancedRenderCommand &b) {
                    if (a.mesh != b.mesh)
                        return a.mesh < b.mesh;
                    if (a.material != b.material)
                        return a.material < b.material;
                    if (a.effectiveTexture != b.effectiveTexture)
                        return a.effectiveTexture < b.effectiveTexture;
                    if (a.color.r != b.color.r)
                        return a.color.r < b.color.r;
                    if (a.color.g != b.color.g)
                        return a.color.g < b.color.g;
                    if (a.color.b != b.color.b)
                        return a.color.b < b.color.b;
                    return a.color.a < b.color.a;
                });
            }

            for (const auto &instCmd : instCmds) {
                if (!instCmd.mesh || !instCmd.material || instCmd.transformCount == 0)
                    continue;
                BatchKey key{instCmd.mesh, instCmd.material, instCmd.effectiveTexture, instCmd.color};
                const glm::mat4 *transformsPtr = instCmd.transformPtr ? instCmd.transformPtr : m_InstancedTransformsStaging[renderIndex].data() + instCmd.transformOffset;
                const glm::vec4 *colorsPtr = instCmd.colorPtr ? instCmd.colorPtr : (instCmd.colorCount > 0 ? m_InstancedColorsStaging[renderIndex].data() + instCmd.colorOffset : nullptr);

                if (instCmd.entityIDPtr || instCmd.entityIDCount > 0) {
                    const int *entityIDsPtr = instCmd.entityIDPtr ? instCmd.entityIDPtr : m_InstancedEntityIDsStaging[renderIndex].data() + instCmd.entityIDOffset;
                    RenderBatch(key, transformsPtr, entityIDsPtr, instCmd.transformCount, renderIndex, colorsPtr);
                } else {
                    if (m_DummyEntityIDs.size() < instCmd.transformCount)
                        m_DummyEntityIDs.resize(instCmd.transformCount, -1);
                    RenderBatch(key, transformsPtr, m_DummyEntityIDs.data(), instCmd.transformCount, renderIndex, colorsPtr);
                }
            }
        }
    }

    // merge and render single commands
    {
        m_BatchRanges.clear();
        m_MergedTransforms.clear();
        m_MergedEntityIDs.clear();
        m_MergedTransforms.reserve(m_Commands[renderIndex].size());
        m_MergedEntityIDs.reserve(m_Commands[renderIndex].size());

        BatchKey currentKey{};
        bool hasCurrent = false;
        size_t batchStart = 0;

        for (const auto &cmd : m_Commands[renderIndex]) {
            if (!cmd.mesh || !cmd.material)
                continue;

            if (BatchKey key{cmd.mesh, cmd.material, cmd.effectiveTexture, cmd.color}; !hasCurrent || currentKey != key) {
                if (hasCurrent)
                    m_BatchRanges.push_back({currentKey, batchStart, m_MergedTransforms.size() - batchStart});
                currentKey = key;
                hasCurrent = true;
                batchStart = m_MergedTransforms.size();
            }
            m_MergedTransforms.push_back(cmd.model);
            m_MergedEntityIDs.push_back(cmd.entityID);
        }
        if (hasCurrent)
            m_BatchRanges.push_back({currentKey, batchStart, m_MergedTransforms.size() - batchStart});

        for (const auto &[key, offset, count] : m_BatchRanges)
            RenderBatch(key, m_MergedTransforms.data() + offset, m_MergedEntityIDs.data() + offset, count, renderIndex);
    }

    m_LastBoundVAO = nullptr;
    m_LastBoundShader = nullptr;
    m_LastBoundTexture = nullptr;
    m_LastBoundColor = glm::vec4(0.0f);

    if (m_PixelReadRequested.load()) {
        if (m_EditorFramebuffer) {
            m_EditorFramebuffer->Bind();
            glReadBuffer(GL_COLOR_ATTACHMENT1);

            int pixelData = -1;
            glReadPixels(m_PixelReadX.load(), m_PixelReadY.load(), 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);

            m_PixelReadResult.store(pixelData);
            m_EditorFramebuffer->Unbind();
        }
        m_PixelReadRequested.store(false);
    }

    m_Commands[renderIndex].clear();
    m_InstancedCommands[renderIndex].clear();
}

void Rendering::Renderer::RenderBatch(const BatchKey &key, const glm::mat4 *transforms, const int *entityIDs, const size_t count, const size_t renderIndex, const glm::vec4 *perInstanceColors) {
    if (count == 0 || count > MAX_INSTANCES)
        return;

    Shader *shader = key.material ? key.material->shader.get() : nullptr;
    if (!shader || !shader->IsValid())
        shader = Shader::Default();

    if (m_LastBoundShader != shader) {
        shader->Bind();
        shader->SetUniformMat4("u_VP", m_VP[renderIndex]);
        BindLightmap(shader, renderIndex);
        m_LastBoundShader = shader;
        // new shader
        m_LastBoundTexture = nullptr;
        m_LastBoundColor = glm::vec4(0.0f);
    }

    // cache texture binding
    if (const Texture *tex = key.texture ? key.texture : Texture::White(); m_LastBoundTexture != tex) {
        tex->Bind(0);
        shader->SetUniform1i("u_Texture", 0);
        m_LastBoundTexture = tex;
    }

    // cache color uniform
    if (m_LastBoundColor != key.color) {
        shader->SetUniformVec4("u_Color", key.color);
        m_LastBoundColor = key.color;
    }

    // find or create VAO for this mesh
    MeshVAO *meshVAOEntry = nullptr;
    for (auto &[mesh, entry] : m_MeshVAOs) {
        if (mesh == key.mesh) {
            meshVAOEntry = &entry;
            break;
        }
    }
    if (!meshVAOEntry) {
        auto vao = std::make_shared<VertexArray>();
        vao->Init();
        vao->Bind();
        vao->AddBuffer(key.mesh->GetVBO(), VertexTraits<Vertex>::GetLayout());
        vao->SetIndexBuffer(key.mesh->GetIBO());
        glBindVertexArray(0);
        m_MeshVAOs.emplace_back(key.mesh, MeshVAO{std::move(vao), false});
        meshVAOEntry = &m_MeshVAOs.back().second;
    }
    const VertexArray *vao = meshVAOEntry->vao.get();

    if (!m_DynamicInstanceBuffer) {
        m_DynamicInstanceBuffer = std::make_unique<VertexBuffer>();
        m_DynamicInstanceBuffer->Init(nullptr, INSTANCE_BUFFER_SIZE, GL_DYNAMIC_DRAW);
    }
    if (!m_DynamicEntityIDBuffer) {
        m_DynamicEntityIDBuffer = std::make_unique<VertexBuffer>();
        m_DynamicEntityIDBuffer->Init(nullptr, ID_BUFFER_SIZE, GL_DYNAMIC_DRAW);
    }
    if (!m_DynamicColorBuffer) {
        m_DynamicColorBuffer = std::make_unique<VertexBuffer>();
        m_DynamicColorBuffer->Init(nullptr, COLOR_BUFFER_SIZE, GL_DYNAMIC_DRAW);
    }

    m_DynamicInstanceBuffer->SetDataOrphaned(transforms, count * sizeof(glm::mat4));
    m_DynamicEntityIDBuffer->SetDataOrphaned(entityIDs, count * sizeof(int));

    // per-instance colors: upload real colors or white defaults
    if (perInstanceColors) {
        m_DynamicColorBuffer->SetDataOrphaned(perInstanceColors, count * sizeof(glm::vec4));
    } else {
        if (m_DefaultInstanceColors.size() < count)
            m_DefaultInstanceColors.assign(count, glm::vec4(1.0f));
        m_DynamicColorBuffer->SetDataOrphaned(m_DefaultInstanceColors.data(), count * sizeof(glm::vec4));
    }

    if (m_LastBoundVAO != vao) {
        vao->Bind();
        m_LastBoundVAO = vao;
    }

    if (!meshVAOEntry->instanceAttribReady) {
        vao->AddInstancedBuffer(*m_DynamicInstanceBuffer, 2);
        vao->AddInstancedIntBuffer(*m_DynamicEntityIDBuffer, 6);

        // attribute 7: per-instance vec4 color
        m_DynamicColorBuffer->Bind();
        glEnableVertexAttribArray(7);
        glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), nullptr);
        glVertexAttribDivisor(7, 1);

        meshVAOEntry->instanceAttribReady = true;
    }

    glDrawElementsInstanced(GL_TRIANGLES, key.mesh->GetIndexCount(), GL_UNSIGNED_INT, nullptr, static_cast<GLsizei>(count));
}

void Rendering::Renderer::Clean() {
    m_Commands[0].clear();
    m_Commands[1].clear();
    m_InstancedCommands[0].clear();
    m_InstancedCommands[1].clear();
    m_InstancedTransformsStaging[0].clear();
    m_InstancedTransformsStaging[1].clear();
    m_InstancedEntityIDsStaging[0].clear();
    m_InstancedEntityIDsStaging[1].clear();
    m_InstancedColorsStaging[0].clear();
    m_InstancedColorsStaging[1].clear();
    m_BatchRanges.clear();
    m_MergedTransforms.clear();
    m_MergedEntityIDs.clear();
    m_DummyEntityIDs.clear();

    m_MeshVAOs.clear();
    m_LastBoundVAO = nullptr;
    m_LastBoundShader = nullptr;
    m_LastBoundTexture = nullptr;
    m_LastBoundColor = glm::vec4(0.0f);
}

void Rendering::Renderer::SetLightmap(const Lightmap *lightmap) { m_Lightmap[m_SubmitIndex] = lightmap; }

void Rendering::Renderer::BindLightmap(Shader *shader, const size_t renderIndex) const {
    if (const Lightmap *lm = m_Lightmap[renderIndex]; lm && lm->framebuffer) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, lm->framebuffer->GetColorAttID());
        shader->SetUniform1i("u_LightTexture", 1);
        shader->SetUniformVec2("u_MapSize", lm->mapSize);
        shader->SetUniformVec2("u_MapOffset", lm->mapOffset);
        shader->SetUniform1f("u_Ambient", lm->ambient);
    } else {
        Texture::White()->Bind(1);
        shader->SetUniform1i("u_LightTexture", 1);
    }
}

void Rendering::Renderer::SetClearColor(const glm::vec4 color) { s_ClearColorStaging = color; }

void Rendering::Renderer::ApplyClearColor() { glClearColor(s_ClearColorStaging[0], s_ClearColorStaging[1], s_ClearColorStaging[2], s_ClearColorStaging[3]); }

void Rendering::Renderer::SwapBuffers() {
    m_RenderIndex = m_SubmitIndex;
    m_SubmitIndex = (m_SubmitIndex + 1) % 2;
}

void Rendering::Renderer::SubmitInitTask(Platform::Threading::SmallTask task) {
    std::lock_guard lock(s_InitQueueMutex);
    s_InitQueue.emplace_back(std::move(task));
    s_HasInitTasks.store(true, std::memory_order_release);
}

void Rendering::Renderer::SubmitInitTask(std::function<void()> task) {
    std::lock_guard lock(s_InitQueueMutex);
    s_InitQueue.emplace_back(std::move(task));
    s_HasInitTasks.store(true, std::memory_order_release);
}

void Rendering::Renderer::ProcessInitQ() {
    if (!s_HasInitTasks.load(std::memory_order_acquire))
        return;
    std::vector<InitTask> queueCopy;
    {
        std::lock_guard lock(s_InitQueueMutex);
        if (s_InitQueue.empty()) {
            s_HasInitTasks.store(false, std::memory_order_release);
            return;
        }
        queueCopy = std::move(s_InitQueue);
        s_HasInitTasks.store(false, std::memory_order_release);
    }
    for (auto &task : queueCopy) {
        std::visit([](auto &t) { t(); }, task);
    }
}

void Rendering::Renderer::EnsureFramebufferSize(uint32_t width, uint32_t height) {
    if (m_FboWidth == width && m_FboHeight == height)
        return;
    m_FboWidth = width;
    m_FboHeight = height;
    SubmitInitTask(Platform::Threading::SmallTask([this, width, height] {
        if (!m_EditorFramebuffer) {
            m_EditorFramebuffer = std::make_shared<FrameBuffer>(width, height);
        } else {
            m_EditorFramebuffer->Invalidate(width, height);
        }
    }));
}
