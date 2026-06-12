#ifndef OBLIBERRY_RENDERER_H
#define OBLIBERRY_RENDERER_H

#include <unordered_map>

#include "Camera.h"
#include "Lightmap.h"
#include "Material.h"
#include "Mesh.h"
#include "Transform.h"
#include "glm/glm.hpp"

struct RenderCommand {
    const Mesh *mesh;
    const Material *material;
    Transform transform;
    const Texture *textureOverride;
    int sortKeyDepth;
    int sortKeyZ;
};

struct InstancedRenderCommand {
    const Mesh *mesh;
    const Material *material;
    const std::vector<glm::mat4> *transforms;
    bool isDirty;
};

class Renderer {
public:
    void SetCamera(const Camera &camera);

    [[nodiscard]] const Camera *GetCamera() const noexcept {
        return m_Camera;
    }

    void BeginFrame();

    void Submit(const Mesh *mesh,
                const Material *material,
                const Transform &transform,
                const Texture *textureOverride = nullptr);

    void Submit(const Mesh *mesh,
                const Material *material,
                const std::vector<glm::mat4> *transforms,
                bool isDirty = true);

    void Flush();

    void InstancedFlush();

    void Clean();

    void SetLightmap(const Lightmap *lightmap);

    void SetClearColor(glm::vec4 color) const;

private:
    void Execute(const RenderCommand &cmd);

    void BindLightmap(Shader *shader) const;

private:
    std::vector<RenderCommand> m_Commands;
    std::vector<InstancedRenderCommand> m_InstancedCommands;
    std::unordered_map<const void *, std::shared_ptr<VertexBuffer> > m_InstanceBuffers;

    struct InstancedGroup {
        std::shared_ptr<VertexBuffer> vbo;
        std::shared_ptr<VertexArray> vao;
    };

    std::unordered_map<const void *, InstancedGroup> m_InstanceGroups;

    const Camera *m_Camera = nullptr;
    const Lightmap *m_Lightmap = nullptr;
    glm::mat4 m_VP;
    glm::vec4 m_ClearColor = {0.0f, 0.0f, 0.0f, 1.0f};
};

#endif //OBLIBERRY_RENDERER_H
