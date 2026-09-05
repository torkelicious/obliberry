#pragma once
#include "Rendering/Types/Camera.h"
#include "Rendering/Types/Material.h"
#include "Rendering/Types/Mesh/Mesh.h"
#include "Rendering/Types/FBO/FrameBuffer.h"
#include "Rendering/Types/Transform.h"
#include "Platform/Threading/SmallTask.h"
#include "PostProcessing/PostProcessing.h"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <variant>
#include <vector>

namespace Rendering {
    struct Lightmap;

    struct LightmapData {
        std::shared_ptr<FrameBuffer> framebuffer = nullptr;
        glm::vec2 mapSize{1.0f, 1.0f};
        glm::vec2 mapOffset{0.0f, 0.0f};
        float ambient = 1.0f;
    };

    struct RenderCommand {
        const Mesh *mesh;
        const Material *material;
        const Texture *effectiveTexture;
        glm::vec4 color;
        glm::mat4 model;
        int32_t sortKey;
        int32_t entityID;
    };

    struct InstancedRenderCommand {
        const Mesh *mesh;
        const Material *material;
        const Texture *effectiveTexture;
        glm::vec4 color;
        int8_t blendMode = 0;
        int32_t renderOrder = 0;
        int8_t shape = 0;

        const glm::mat4 *transformPtr = nullptr;
        size_t transformOffset = 0;
        size_t transformCount = 0;

        const glm::vec4 *colorPtr = nullptr;
        size_t colorOffset = 0;
        size_t colorCount = 0;

        const int32_t *entityIDPtr = nullptr;
        size_t entityIDOffset = 0;
        size_t entityIDCount = 0;
    };

    struct BatchKey {
        const Mesh *mesh;
        const Material *material;
        const Texture *texture;
        glm::vec4 color;
        int32_t shape = 0;
        bool operator==(const BatchKey &other) const noexcept { return mesh == other.mesh && material == other.material && texture == other.texture && color == other.color && shape == other.shape; }
    };

    class Renderer {
    public:
        void SetCamera(const Camera &camera, float aspect);

        [[nodiscard]] const Camera *GetCamera() const noexcept { return m_Camera; }
        [[nodiscard]] const glm::mat4 &GetCurrentVP() const noexcept { return m_VP[m_SubmitIndex]; }

        void BeginFrame();

        void Submit(const std::shared_ptr<Mesh> &mesh, const std::shared_ptr<Material> &material, const Transform &transform, const Texture *textureOverride = nullptr, int32_t entityID = -1);
        void Submit(const std::shared_ptr<Mesh> &mesh, const std::shared_ptr<Material> &material, const std::vector<glm::mat4> &transforms, const std::vector<int32_t> &entityIDs = {});
        void Submit(const std::shared_ptr<Mesh> &mesh, const std::shared_ptr<Material> &material, const std::vector<glm::mat4> &transforms, const std::vector<glm::vec4> &colors, int32_t blendMode = 0,
                    int32_t renderOrder = 0, int8_t shape = 0);
        void SubmitPersistent(const std::shared_ptr<Mesh> &mesh, const std::shared_ptr<Material> &material, const std::vector<glm::mat4> *transforms, const std::vector<int32_t> *entityIDs = nullptr);

        template <typename T> void Pin(const std::shared_ptr<T> &resource) {
            if (resource)
                m_ResourcePins[m_SubmitIndex].push_back(resource);
        }

        void Flush(size_t renderIndex);
        void Clean();
        void InvalidateGLCache();
        void SwapBuffers();

        void SetLightmap(const Lightmap *lightmap);

        static void SetClearColor(glm::vec4 color);
        static void ApplyClearColor();

        static void SubmitInitTask(Platform::Threading::SmallTask task);
        static void SubmitInitTask(std::function<void()> task);
        static void ProcessInitQ();

        void SetFallbackShader(Shader *shader) { m_FallbackShader = shader; }

        void RequestPixelRead(const int x, const int y) {
            m_PixelReadX.store(x);
            m_PixelReadY.store(y);
            m_PixelReadRequested.store(true);
        }

        [[nodiscard]] int GetLastReadPixel() const { return m_PixelReadResult.load(); }
        [[nodiscard]] bool IsPixelReadRequested() const { return m_PixelReadRequested.load(); }
        void ClearPixelReadResult() { m_PixelReadResult.store(-1); }

        void SetEditorMode(const bool editor) { m_EditorMode = editor; }
        [[nodiscard]] bool IsEditorMode() const { return m_EditorMode; }

        void EnsureSceneFramebufferSize(uint32_t width, uint32_t height);
        [[nodiscard]] std::shared_ptr<FrameBuffer> GetSceneFrameBuffer() const { return m_SceneFrameBuffer; }
        [[nodiscard]] PostProcessing::PostProcessor &GetPostProcessor() { return m_PostProcessor; }
        void SetPassthroughShader(std::shared_ptr<Shader> s) { m_PassthroughShader = std::move(s); }

        void RunPostProc();
        void PresentToScreen(uint32_t width, uint32_t height);

    private:
        void BindLightmap(Shader *shader, size_t renderIndex) const;
        void DrawFullscreenPassthrough(uint32_t coltex, uint32_t width, uint32_t height, FrameBuffer *target) const;
        void RenderBatch(const BatchKey &key, const glm::mat4 *transforms, const int32_t *entityIDs, size_t count, size_t renderIndex, const glm::vec4 *perInstanceColors = nullptr);

        std::vector<std::shared_ptr<void>> m_ResourcePins[2];

        using InitTask = std::variant<Platform::Threading::SmallTask, std::function<void()>>;
        static std::vector<InitTask> s_InitQueue;
        static std::mutex s_InitQueueMutex;
        static std::atomic<bool> s_HasInitTasks;

        size_t m_SubmitIndex = 0;
        size_t m_RenderIndex = 1;

        std::vector<RenderCommand> m_Commands[2];
        std::vector<InstancedRenderCommand> m_InstancedCommands[2];

        std::vector<glm::mat4> m_InstancedTransformsStaging[2];
        std::vector<int32_t> m_InstancedEntityIDsStaging[2];
        std::vector<glm::vec4> m_InstancedColorsStaging[2];

        const Camera *m_Camera = nullptr;
        LightmapData m_Lightmap[2];

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
        std::unique_ptr<VertexBuffer> m_DynamicColorBuffer;
        std::vector<glm::vec4> m_DefaultInstanceColors;
        const VertexArray *m_LastBoundVAO = nullptr;
        const Shader *m_LastBoundShader = nullptr;
        const Texture *m_LastBoundTexture = nullptr;
        glm::vec4 m_LastBoundColor{0.0f};

        std::vector<BatchRange> m_BatchRanges;
        std::vector<glm::mat4> m_MergedTransforms;
        std::vector<int32_t> m_MergedEntityIDs;
        std::vector<int32_t> m_DummyEntityIDs;

        Shader *m_FallbackShader = nullptr;

        std::atomic<bool> m_PixelReadRequested{false};
        std::atomic<int> m_PixelReadX{0};
        std::atomic<int> m_PixelReadY{0};
        std::atomic<int32_t> m_PixelReadResult{-1};

        // post-proc
        std::shared_ptr<FrameBuffer> m_SceneFrameBuffer;
        std::shared_ptr<FrameBuffer> m_PingPong[2]; // multi pass effects
        PostProcessing::PostProcessor m_PostProcessor;
        std::shared_ptr<Shader> m_PassthroughShader;
        uint32_t m_PPWidth = 0;
        uint32_t m_PPHeight = 0;
        bool m_EditorMode = false;
    };
} // namespace Rendering
