

#ifndef ISOMETRICGAME_ACTOR_H
#define ISOMETRICGAME_ACTOR_H
#include "ECS/Components.h"

class Actor {
public:
    Position GridPosition; // add world pos later if we implement "world" map?
    bool isPlayer;

private:
    // TODO: movement grid stuff bla blah world pos blah blah
};


#endif //ISOMETRICGAME_ACTOR_H
