#ifndef ISOMETRICGAME_ACTOR_H
#define ISOMETRICGAME_ACTOR_H
#include "ECS/Components.h"

class Actor {
public:
    Actor() {
        GridPosition = {0.0f, 0.0f};
        isPlayer = false;
        sprite = {};
    }

    Position GridPosition; // add world pos later if we implement "world" map?
    bool isPlayer;
    Sprite sprite;
    void move(glm::vec2 movement);

private:
    // TODO: movement grid stuff bla blah world pos blah blah
};


#endif //ISOMETRICGAME_ACTOR_H
