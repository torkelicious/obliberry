#ifndef ISOMETRICGAME_COMPONENTS_H
#define ISOMETRICGAME_COMPONENTS_H
#include "glm/glm.hpp"

// Screen position / scale
struct Transform {
    glm::vec2 Position;
    glm::vec2 Scale;
    float rotation = 0.0f; // radians
};

// World map position
struct Position {
    float x, y;
};

/*
struct Velocity {
    float x, y;
    bool active = false;
};
*/

struct Sprite {
    GLuint textureID;
};

// collider , actor, stats, inv etc later?

#endif //ISOMETRICGAME_COMPONENTS_H
