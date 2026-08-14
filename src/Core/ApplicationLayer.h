#pragma once
#include "Core/EngineContext.h"

namespace Core {
    class ApplicationLayer {
    public:
        virtual ~ApplicationLayer() = default;

        virtual void Init(EngineContext &ctx) {}

        virtual void PreImGuiFrame() {} // mayb move away type shit im tird

        virtual void Update(float dt) {}

        virtual void Render() {}

        virtual void Shutdown() {}
    };
} // namespace Core
