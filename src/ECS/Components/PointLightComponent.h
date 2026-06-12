

#ifndef OBLIBERRY_POINTLIGHTCOMPONENT_H
#define OBLIBERRY_POINTLIGHTCOMPONENT_H
#include <glm/glm.hpp>

struct PointLightComponent {
    glm::vec3 color = {1.0f, 1.0f, 1.0f};
    float radius = 50.0f; // world space radius
    float intensity = 1.0f;
};

#endif //OBLIBERRY_POINTLIGHTCOMPONENT_H
