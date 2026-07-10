#pragma once
#include "Core/EngineContext.h"


#include <string_view>

namespace Editor {
    // Editor command pattern
    // used for undo/redo
    class ICommand {
    public:
        virtual ~ICommand() = default;
        virtual void Execute(Core::EngineContext &ctx) = 0;
        virtual void Undo(Core::EngineContext &ctx) = 0;
        virtual std::string_view Name() const noexcept = 0;
    };
} // namespace Editor
