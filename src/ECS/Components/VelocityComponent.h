#ifndef ISOMETRICGAME_VELOCITYCOMPONENT_H
#define ISOMETRICGAME_VELOCITYCOMPONENT_H

#include "ECS/ECS.h"
#include "glm/glm.hpp"

struct Velocity : public Component {
    glm::vec2 Value{0.0f, 0.0f};
};

#endif //ISOMETRICGAME_VELOCITYCOMPONENT_H

