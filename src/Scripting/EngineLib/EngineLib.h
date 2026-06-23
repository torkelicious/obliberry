#pragma once

#include "Core/EngineContext.h"
#include "ECS/Registry.h"
#include "Scripting/ObSLCore/StdLib/StdLib.h"

class EngineLib : public ObSL::Lib {
public:
    void register_enginelib(ObSL::Interpreter &interpreter, Registry &registry, EngineContext &ctx) {
        m_ctx = &ctx;
        m_registry = &registry;
        register_modules(interpreter);
    }

private:
    // ReSharper disable once CppOverrideWithDifferentVisibility
    void register_modules(ObSL::Interpreter &interpreter) override;

    // submodules
    void register_core_modules(ObSL::Interpreter &interpreter);

    void register_input_modules(ObSL::Interpreter &interpreter);

    void register_camera_modules(ObSL::Interpreter &interpreter);

    void register_map_modules(ObSL::Interpreter &interpreter);

    void register_audio_modules(ObSL::Interpreter &interpreter);

    void register_scene_management_modules(ObSL::Interpreter &interpreter);

    Registry *m_registry = nullptr;
    EngineContext *m_ctx = nullptr;
};

