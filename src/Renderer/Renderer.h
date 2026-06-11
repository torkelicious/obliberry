#ifndef OBLIBERRY_RENDERER_H
#define OBLIBERRY_RENDERER_H

#include <unordered_map>

#include "Camera.h"
#include "Material.h"
#include "Mesh.h"
#include "Transform.h"
#include "glm/glm.hpp"

struct RenderCommand {
    std::shared_ptr<const Mesh> mesh;
    std::shared_ptr<const Material> material;
    Transform transform;
    int sortKeyDepth;
    int sortKeyZ;
};

struct InstancedRenderCommand {
    std::shared_ptr<const Mesh> mesh;
    std::shared_ptr<const Material> material;
    const std::vector<glm::mat4> *transforms;
    bool isDirty;
};

class Renderer {
public:
    void SetCamera(const Camera &camera);

    const Camera *GetCamera() {
        return m_Camera;
    }

    void BeginFrame();

    void Submit(const std::shared_ptr<const Mesh> &mesh,
                const std::shared_ptr<const Material> &material,
                const Transform &transform);

    void Submit(const std::shared_ptr<const Mesh> &mesh,
                const std::shared_ptr<const Material> &material,
                const std::vector<glm::mat4> *transforms,
                bool isDirty = true);

    void Flush();

    void InstancedFlush();

    void Clean();

private:
    void Execute(const RenderCommand &cmd);

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
    glm::mat4 m_VP;
};

#endif //OBLIBERRY_RENDERER_H
