

#ifndef ISOMETRICGAME_MATERIALCOMPONENT_H
#define ISOMETRICGAME_MATERIALCOMPONENT_H
#include "ECS/Components.h"
#include "Graphics/Material.h"

struct MaterialComponent : public Component {
    Material *material = nullptr;
};

#endif //ISOMETRICGAME_MATERIALCOMPONENT_H
