#ifndef ISOMETRICGAME_RENDERER_H
#define ISOMETRICGAME_RENDERER_H
#include "Camera.h"
#include "Mesh.h"
#include "Material.h"

struct RenderCommand {
    const Mesh *mesh;
    const Material *material;
    Transform transform;
};

class Renderer {
public:
    void BeginFrame(const Camera &camera);

    void Submit(const Mesh &mesh, const Material &material, const Transform &transform);

    void Flush();

private:
    const Camera *m_Camera = nullptr;
    std::vector<RenderCommand> m_Commands;

    void Execute(const RenderCommand &cmd);
};

#endif //ISOMETRICGAME_RENDERER_H
