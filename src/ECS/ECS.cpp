#include "ECS.h"


Entity::Entity() = default;

Entity::~Entity() = default;

void Entity::Update(float dt) {
    for (auto &component: m_Components) {
        component->UpdateComponent(dt, *this);
    }
}

void Entity::Render() {
    for (auto &component: m_Components) {
        component->Render();
    }
}
