#ifndef ISOMETRICGAME_SPRITECOMPONENT_H
#define ISOMETRICGAME_SPRITECOMPONENT_H
#include "ECS/Components.h"
#include "Graphics/Texture.h"

struct Sprite : public Component {
    Transform SpriteTransform{}; // sprite local transform compared to model transform
    Texture *SpriteTexture = nullptr;

    // make sure to link m_ID to texture slot and id sorting for objects too ?
    void UpdateComponent(float dt, Entity &entity) override;

    void Render() override;
};
#endif //ISOMETRICGAME_SPRITECOMPONENT_H
