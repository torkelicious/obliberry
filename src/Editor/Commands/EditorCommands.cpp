#include "EditorCommands.h"
#include "ECS/Entity.h"
#include "ECS/Components/TransformComponent.h"
#include "Scenes/SceneManager.h"
namespace Editor::Commands {

    // Move Transform
    TranslateEntityCommand::TranslateEntityCommand(const ECS::EntityID target, const glm::vec3 oldPos,
                                                   const glm::vec3 newPos)
        : m_EntityID(target), m_OldPos(oldPos), m_NewPos(newPos) {}

    void TranslateEntityCommand::Execute(Core::EngineContext &ctx) {
        const ECS::Entity ent(m_EntityID, &ctx.sceneManager->GetCurrentScene()->GetRegistry());
        ent.GetComponent<ECS::Components::TransformComponent>()->transform.SetPosition(m_NewPos);
    }

    void TranslateEntityCommand::Undo(Core::EngineContext &ctx) {
        const ECS::Entity ent(m_EntityID, &ctx.sceneManager->GetCurrentScene()->GetRegistry());
        ent.GetComponent<ECS::Components::TransformComponent>()->transform.SetPosition(m_OldPos);
    }
    std::string_view TranslateEntityCommand::Name() const noexcept { return "Move entity"; }


    // Rotate Transform
    RotateEntityCommand::RotateEntityCommand(const ECS::EntityID target, const glm::vec3 oldRot, const glm::vec3 newRot)
        : m_EntityID(target), m_OldRot(oldRot), m_NewRot(newRot) {}
    void RotateEntityCommand::Execute(Core::EngineContext &ctx) {
        const ECS::Entity ent(m_EntityID, &ctx.sceneManager->GetCurrentScene()->GetRegistry());
        ent.GetComponent<ECS::Components::TransformComponent>()->transform.SetRotation(m_NewRot);
    }
    void RotateEntityCommand::Undo(Core::EngineContext &ctx) {
        const ECS::Entity ent(m_EntityID, &ctx.sceneManager->GetCurrentScene()->GetRegistry());
        ent.GetComponent<ECS::Components::TransformComponent>()->transform.SetRotation(m_OldRot);
    }
    std::string_view RotateEntityCommand::Name() const noexcept { return "Rotate entity"; }

    // Scale Transform
    ScaleEntityCommand::ScaleEntityCommand(const ECS::EntityID target, const glm::vec3 oldScale,
                                           const glm::vec3 newScale)
        : m_EntityID(target), m_OldScale(oldScale), m_NewScale(newScale) {}
    void ScaleEntityCommand::Execute(Core::EngineContext &ctx) {
        const ECS::Entity ent(m_EntityID, &ctx.sceneManager->GetCurrentScene()->GetRegistry());
        ent.GetComponent<ECS::Components::TransformComponent>()->transform.SetScale(m_NewScale);
    }
    void ScaleEntityCommand::Undo(Core::EngineContext &ctx) {
        const ECS::Entity ent(m_EntityID, &ctx.sceneManager->GetCurrentScene()->GetRegistry());
        ent.GetComponent<ECS::Components::TransformComponent>()->transform.SetScale(m_OldScale);
    }
    std::string_view ScaleEntityCommand::Name() const noexcept {
        return "Scale entity";
    }

} // namespace Editor::Commands
