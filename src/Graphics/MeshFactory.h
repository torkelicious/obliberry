

#ifndef ISOMETRICGAME_MESHFACTORY_H
#define ISOMETRICGAME_MESHFACTORY_H


#include "Mesh.h"

class MeshFactory {
public:
    static Mesh CreateQuad();

    static Mesh CreateFlatTopHex(float rad = 1.0f);


    static Mesh CreatePointTopHex(float rad = 1.0f);

    static Mesh CreateTriangle();

    // todo: add cachiong later
};


#endif //ISOMETRICGAME_MESHFACTORY_H
