#ifndef ISOMETRICGAME_ENTITY_H
#define ISOMETRICGAME_ENTITY_H
#include "Graphics/Mesh.h"
#include "Graphics/Shader.h"
#include "Graphics/Texture.h"

using EntityID = uint64_t;

class Entity {
public:
    EntityID ID;
    Transform Transform;
    //Mesh* Mesh = nullptr;
    //Shader* Shader = nullptr;
    //Texture* Texture = nullptr;
    virtual ~Entity() = default;
};


#endif //ISOMETRICGAME_ENTITY_H
