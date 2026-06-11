#ifndef OBLIBERRY_RENDERER_H
#define OBLIBERRY_RENDERER_H

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

class Renderer {
public:
    void SetCamera(const Camera &camera);

    const Camera *GetCamera() {
        return m_Camera;
    }

    void BeginFrame();

    void Submit(std::shared_ptr<const Mesh> mesh,
                std::shared_ptr<const Material> material,
                const Transform &transform);

    void Flush();

    void Clean();

private:
    void Execute(const RenderCommand &cmd);

private:
    std::vector<RenderCommand> m_Commands;
    const Camera *m_Camera = nullptr;
    glm::mat4 m_VP;
};


#endif //OBLIBERRY_RENDERER_H
