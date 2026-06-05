

#ifndef ISOMETRICGAME_MESHFACTORY_H
#define ISOMETRICGAME_MESHFACTORY_H


#include "Mesh.h"

class MeshFactory {
public:
    static MeshData CreateQuad();

    static MeshData CreatePointTopHex(float rad = 1.0f);

    static MeshData CreateTriangle();

    // todo: add cachiong later
};


#endif //ISOMETRICGAME_MESHFACTORY_H
