#ifndef OBLIBERRY_ENGINELIB_H
#define OBLIBERRY_ENGINELIB_H
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

    Registry *m_registry = nullptr;
    EngineContext *m_ctx = nullptr;
};

#endif //OBLIBERRY_ENGINELIB_H
