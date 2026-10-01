#include <gtest/gtest.h>
#include "ECS/Systems/Collision/CollisionFilter.h"

TEST(ColliderTests, BothMasksMustAcceptTheOtherLayer) {
    ECS::Components::ColliderComponent a;
    ECS::Components::ColliderComponent b;

    a.layer = 2;
    b.layer = 3;

    a.mask = 1u << 3;
    b.mask = 1u << 2;

    EXPECT_TRUE(ECS::Collision::shouldCollide(a, b));

    b.mask = 0;

    EXPECT_FALSE(ECS::Collision::shouldCollide(a, b));
    EXPECT_FALSE(ECS::Collision::shouldCollide(b, a));
}
