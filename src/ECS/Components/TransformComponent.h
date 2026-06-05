#ifndef ISOMETRICGAME_TRANSFORMCOMPONENT_H
#define ISOMETRICGAME_TRANSFORMCOMPONENT_H
#include "ECS/ECS.h"
#include "glm/glm.hpp"

// Screen position / scale
struct Transform : public Component {
    glm::vec3 Position{1.0f, 1.0f, 1.0f};
    glm::vec2 Scale{1.0f, 1.0f};
    float rotation = 0.0f; // radians
};

#endif //ISOMETRICGAME_TRANSFORMCOMPONENT_H
