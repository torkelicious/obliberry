#include "Actor.h"

void Actor::move(glm::vec2 movement) {
    GridPosition.x += movement.x;
    GridPosition.y += movement.y;
}
