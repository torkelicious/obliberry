#ifndef OBLIBERRY_ECS_H
#define OBLIBERRY_ECS_H

/*
 * refs/inspo:
 *   https://en.cppreference.com/cpp/container/unordered_map
 *   https://www.geeksforgeeks.org/dsa/sparse-set/
 *   https://austinmorlan.com/posts/entity_component_system/
 *   https://www.codingwiththomas.com/blog/an-entity-component-system-from-scratch
 *   https://www.david-colson.com/2020/02/09/making-a-simple-ecs.html
 *
 * Roadmap:
 *   ComponentPool currently uses std::unordered_map which has high heap overhead
 *   (bucket allocations, 32-byte node tracking).
 *   could use contiguous flat array with a sparse to dense index mapping for better cache
 *   performance and lower allocation pressure.
 */


#include "Types.h"
#include "IPool.h"
#include "ComponentPool.h"
#include "Registry.h"
#include "Entity.h"

#endif //OBLIBERRY_ECS_H
