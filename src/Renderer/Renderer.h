#ifndef OBLIBERRY_RENDERER_H
#define OBLIBERRY_RENDERER_H

#include <unordered_set>

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
};


class Renderer {
public:
    Renderer();

    void SetCamera(const Camera &camera);

    const Camera *GetCamera() {
        return m_Camera;
    }

    void BeginFrame();

    void Submit(std::shared_ptr<const Mesh> mesh,
                std::shared_ptr<const Material> material,
                const Transform &transform);

    // instanced calls
    void Submit(std::shared_ptr<const Mesh> mesh,
                std::shared_ptr<const Material> material,
                const std::vector<glm::mat4> *transforms);


    void Flush();

    void InstancedFlush();

    void Clean();

private:
    void Execute(const RenderCommand &cmd);

private:
    std::vector<RenderCommand> m_Commands;
    std::vector<InstancedRenderCommand> m_InstancedCommands;
    std::unordered_set<GLuint> m_ConfiguredInstancedVAOs;
    std::shared_ptr<VertexBuffer> m_InstanceBuffer;
    const Camera *m_Camera = nullptr;
    glm::mat4 m_VP;
};


#endif //OBLIBERRY_RENDERER_H
