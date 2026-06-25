#pragma once
#include "Core/EngineContext.h"

class ApplicationLayer {
public:
    virtual ~ApplicationLayer() = default;

    virtual void Init(EngineContext &ctx) {
    }

    virtual void Update(float dt) {
    }

    virtual void Render() {
    }

    virtual void Shutdown() {
    }
};
