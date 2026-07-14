#pragma once
#include "Camera.h"
#include "Lightmap.h"
#include "Material.h"
#include "Mesh.h"
#include "FrameBuffer.h"
#include "Transform.h"
#include "Core/SmallTask.h"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <variant>
#include <vector>

namespace Rendering {
    struct RenderCommand {
        const Mesh *mesh;
        const Material *material;
        const Texture *effectiveTexture;
        glm::vec4 color;
        glm::mat4 model;
        int32_t sortKey;
        int entityID; // signed int since -1 is used to represent invalid/non-entities
    };

    struct InstancedRenderCommand {
        const Mesh *mesh;
        const Material *material;
        const Texture *effectiveTexture;
        glm::vec4 color;

        const glm::mat4 *transformPtr = nullptr;
        size_t transformOffset = 0;
        size_t transformCount = 0;

        const int *entityIDPtr = nullptr;
        size_t entityIDOffset = 0;
        size_t entityIDCount = 0;
    };

    struct BatchKey {
        const Mesh *mesh;
        const Material *material;
        const Texture *texture;
        glm::vec4 color;
        bool operator==(const BatchKey &other) const noexcept { return mesh == other.mesh && material == other.material && texture == other.texture && color == other.color; }
    };

    class Renderer {
    public:
        void SetCamera(const Camera &camera, float aspect);

        [[nodiscard]] const Camera *GetCamera() const noexcept { return m_Camera; }
        [[nodiscard]] const glm::mat4 &GetCurrentVP() const noexcept { return m_VP[m_SubmitIndex]; }

        void BeginFrame();

        void Submit(const std::shared_ptr<Mesh> &mesh, const Material *material, const Transform &transform, const Texture *textureOverride = nullptr, int entityID = -1);

        void Submit(const std::shared_ptr<Mesh> &mesh, const Material *material, const std::vector<glm::mat4> &transforms, const std::vector<int> &entityIDs = {});

        void SubmitPersistent(const std::shared_ptr<Mesh> &mesh, const Material *material, const std::vector<glm::mat4> *transforms, const std::vector<int> *entityIDs = nullptr);

        void Flush(size_t renderIndex);

        void Clean();

        void SwapBuffers();

        void SetLightmap(const Lightmap *lightmap);

        static void SetClearColor(glm::vec4 color);

        static void ApplyClearColor();

        static void SubmitInitTask(Core::SmallTask task);
        static void SubmitInitTask(std::function<void()> task);

        static void ProcessInitQ();

        [[nodiscard]] std::shared_ptr<FrameBuffer> GetEditorFramebuffer() const { return m_EditorFramebuffer; }

        void EnsureFramebufferSize(uint32_t width, uint32_t height);

        // Picking
        void RequestPixelRead(const int x, const int y) {
            m_PixelReadX.store(x);
            m_PixelReadY.store(y);
            m_PixelReadRequested.store(true);
        }

        [[nodiscard]] int GetLastReadPixel() const { return m_PixelReadResult.load(); }
        [[nodiscard]] bool IsPixelReadRequested() const { return m_PixelReadRequested.load(); }
        void ClearPixelReadResult() { m_PixelReadResult.store(-1); }

    private:
        void BindLightmap(Shader *shader, size_t renderIndex) const;

        void RenderBatch(const BatchKey &key, const glm::mat4 *transforms, const int *entityIDs, size_t count, size_t renderIndex);

        using InitTask = std::variant<Core::SmallTask, std::function<void()>>;
        static std::vector<InitTask> s_InitQueue;
        static std::mutex s_InitQueueMutex;
        static std::atomic<bool> s_HasInitTasks;

        size_t m_SubmitIndex = 0;
        size_t m_RenderIndex = 1;

        std::vector<RenderCommand> m_Commands[2];
        std::vector<InstancedRenderCommand> m_InstancedCommands[2];

        std::vector<glm::mat4> m_InstancedTransformsStaging[2];
        std::vector<int> m_InstancedEntityIDsStaging[2];

        const Camera *m_Camera = nullptr;
        const Lightmap *m_Lightmap[2] = {nullptr, nullptr};

        float m_Aspect = 1.7777777f;
        glm::mat4 m_VP[2] = {glm::mat4(1.0f), glm::mat4(1.0f)};

        struct MeshVAO {
            std::shared_ptr<VertexArray> vao;
            bool instanceAttribReady = false;
        };

        struct BatchRange {
            BatchKey key;
            size_t offset;
            size_t count;
        };

        std::vector<std::pair<const Mesh *, MeshVAO>> m_MeshVAOs;
        std::unique_ptr<VertexBuffer> m_DynamicInstanceBuffer;
        std::unique_ptr<VertexBuffer> m_DynamicEntityIDBuffer;
        const VertexArray *m_LastBoundVAO = nullptr;
        const Shader *m_LastBoundShader = nullptr;
        const Texture *m_LastBoundTexture = nullptr;
        glm::vec4 m_LastBoundColor{0.0f};

        // per frame merge buffers
        std::vector<BatchRange> m_BatchRanges;
        std::vector<glm::mat4> m_MergedTransforms;
        std::vector<int> m_MergedEntityIDs;
        std::vector<int> m_DummyEntityIDs;

        std::shared_ptr<FrameBuffer> m_EditorFramebuffer = nullptr;
        uint32_t m_FboWidth = 0;
        uint32_t m_FboHeight = 0;

        // picking
        std::atomic<bool> m_PixelReadRequested{false};
        std::atomic<int> m_PixelReadX{0};
        std::atomic<int> m_PixelReadY{0};
        std::atomic<int> m_PixelReadResult{-1};
    };
} // namespace Rendering
