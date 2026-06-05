#include "SpriteComponent.h"

void Sprite::UpdateComponent(float dt, Entity &entity) {
    Component::UpdateComponent(dt, entity);
}

void Sprite::Render() {
    Component::Render();
}
