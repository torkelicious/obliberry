

#ifndef OBLIBERRY_IPOOL_H
#define OBLIBERRY_IPOOL_H
#include "Types.h"

// base class for stuff like component pools
class IPool {
public:
    virtual ~IPool() = default;

    virtual void EntityDestroyed(EntityID entity) = 0;
};

#endif //OBLIBERRY_IPOOL_H
