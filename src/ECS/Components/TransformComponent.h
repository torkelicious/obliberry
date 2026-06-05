#ifndef ISOMETRICGAME_TRANSFORMCOMPONENT_H
#define ISOMETRICGAME_TRANSFORMCOMPONENT_H
#include "ECS/ECS.h"
#include "glm/glm.hpp"

// Screen position / scale
struct Transform : public Component {
    glm::vec3 Position{0.0f, 0.0f, 0.0f};
    glm::vec3 Scale{1.0f, 1.0f, 1.0f};
    glm::vec3 Rotation{0.0f, 0.0f, 0.0f};
};

#endif //ISOMETRICGAME_TRANSFORMCOMPONENT_H
