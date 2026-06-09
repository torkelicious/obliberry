#ifndef OBLIBERRY_ECS_H
#define OBLIBERRY_ECS_H

/*
refs/inspo:
https://en.cppreference.com/cpp/container/unordered_map
https://www.geeksforgeeks.org/dsa/sparse-set/
https://austinmorlan.com/posts/entity_component_system/
https://www.codingwiththomas.com/blog/an-entity-component-system-from-scratch
https://www.david-colson.com/2020/02/09/making-a-simple-ecs.html


TODO:
 *    high heap overhead from `std::unordered_map`
 *    bucket allocations and 32-byte node tracking.
 *    refactor ComponentPool away from standard hash maps, use contiguous, page-backed flat array
 *
 *    remove the public getter GetComponentPools(),
 *    implement internal iterator pattern or let Registry populate
 */


#include "Types.h"
#include "IPool.h"
#include "ComponentPool.h"
#include "Registry.h"
#include "Entity.h"

#endif //OBLIBERRY_ECS_H
