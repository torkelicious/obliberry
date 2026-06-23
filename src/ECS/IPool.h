#pragma once

#include "Types.h"

// base class for stuff like component pools
class IPool {
public:
    virtual ~IPool() = default;

    virtual void EntityDestroyed(EntityID entity) = 0;
};
