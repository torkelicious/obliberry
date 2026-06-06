#ifndef OBLIBERRY_RENDERER_H
#define OBLIBERRY_RENDERER_H

#include "Camera.h"
#include "Material.h"
#include "Mesh.h"
#include "Transform.h"
#include "glm/glm.hpp"

glm::mat4 TransformToMatrix(const Transform &t);

struct RenderCommand {
    const Mesh *mesh;
    const Material *material;
    Transform transform;
};

class Renderer {
public:
    void SetCamera(const Camera &camera, float width, float height);

    void BeginFrame();

    void Submit(const Mesh &mesh,
                const Material &material,
                const Transform &transform);

    void Flush();

private:
    void Execute(const RenderCommand &cmd);

private:
    std::vector<RenderCommand> m_Commands;
    const Camera *m_Camera = nullptr;
    glm::mat4 m_VP;
};


#endif //OBLIBERRY_RENDERER_H
