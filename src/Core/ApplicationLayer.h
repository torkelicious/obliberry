#pragma once
#include "Core/EngineContext.h"

namespace Core {
    class ApplicationLayer {
    public:
        virtual ~ApplicationLayer() = default;

        virtual void Init(EngineContext &ctx) {}

        virtual void SetupFontSync(std::atomic<bool> *fontsDirty) {}

        virtual void PreImGuiFrame() {}

        virtual void SyncFonts(EngineContext &ctx, std::mutex &imguiTextureMutex) {}

        virtual void Update(float dt) {}

        virtual void Render() {}

        virtual void Shutdown() {}

        virtual bool UsesEditorViewport() const { return false; }
    };
} // namespace Core
