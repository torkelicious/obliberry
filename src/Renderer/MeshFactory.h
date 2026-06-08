

#ifndef OBLIBERRY_MESHFACTORY_H
#define OBLIBERRY_MESHFACTORY_H
#include "Mesh.h"


class MeshFactory {
public:
    static MeshData CreateQuad();

    static MeshData CreatePointTopHex(float size = 0.5f);

    static MeshData CreateTriangle();

    static MeshData CreateStandingQuad(float width, float height);
};


#endif //OBLIBERRY_MESHFACTORY_H
