

#ifndef ISOMETRICGAME_RENDERER_H
#define ISOMETRICGAME_RENDERER_H
#include "Mesh.h"
#include "Shader.h"


class Renderer {
public:
    //              vec4 curr for pos, pos, size, size
    void Draw(const Mesh &mesh, Shader &shader, const Transform &transform, const glm::mat4 &vp);

    void Clear();
};


#endif //ISOMETRICGAME_RENDERER_H
