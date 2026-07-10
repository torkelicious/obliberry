#pragma once
#include "ICommand.h"
#include <glm/glm.hpp>
#include "ECS/Types.h"

namespace Editor::Commands {
    //
    // Move
    //

    class TranslateEntityCommand final : public ICommand {
    public:
        // i could make this a unified transform command
        // but transforms are much larger than just the vec3's they hold sooo...
        // optimization ig :DDDD
        TranslateEntityCommand(ECS::EntityID target, glm::vec3 oldPos, glm::vec3 newPos);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        ECS::EntityID m_EntityID;
        glm::vec3 m_OldPos;
        glm::vec3 m_NewPos;
    };

    //
    // Rotate
    //

    class RotateEntityCommand final : public ICommand {
    public:
        RotateEntityCommand(ECS::EntityID target, glm::vec3 oldRot, glm::vec3 newRot);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        ECS::EntityID m_EntityID;
        glm::vec3 m_OldRot;
        glm::vec3 m_NewRot;
    };

    //
    // Scale
    //

    class ScaleEntityCommand final : public ICommand {
    public:
        ScaleEntityCommand(ECS::EntityID target, glm::vec3 oldScale, glm::vec3 newScale);
        void Execute(Core::EngineContext &ctx) override;
        void Undo(Core::EngineContext &ctx) override;
        [[nodiscard]] std::string_view Name() const noexcept override;

    private:
        ECS::EntityID m_EntityID;
        glm::vec3 m_OldScale;
        glm::vec3 m_NewScale;
    };


} // namespace Editor::Commands
