#include "../EngineLib.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Systems/MovementSystem.h"
#include "Math/HexMath.h"
#include "Scripting/ObSLCore/Interpreter/Interpreter.h"

void EngineLib::register_map_modules(ObSL::Interpreter &interpreter) {
    interpreter.get_global_environment()->define(
        "GetSelectedHex", interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [reg = m_registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
                auto *obj = interp->gc.allocate<ObSL::ObSLObject>();
                obj->fields["hasSelection"] = false;
                obj->fields["q"] = 0.0;
                obj->fields["r"] = 0.0;
                reg->ForEach<MapStateComponent>(
                    [&](Entity, const MapStateComponent *state) {
                        if (state->hasSelection) {
                            obj->fields["hasSelection"] = true;
                            obj->fields["q"] = static_cast<double>(state->selectedHex.q);
                            obj->fields["r"] = static_cast<double>(state->selectedHex.r);
                        }
                    });
                return obj;
            }, "GetSelectedHex"));

    interpreter.get_global_environment()->define(
        "SetPathToHex", interpreter.gc.allocate<ObSL::NativeFunction>(
            3,
            [reg = m_registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (args.size() < 3 || !std::holds_alternative<double>(args[0])
                    || !std::holds_alternative<double>(args[1])
                    || !std::holds_alternative<double>(args[2]))
                    return false;

                const auto id = static_cast<EntityID>(std::get<double>(args[0]));
                const int targetQ = static_cast<int>(std::get<double>(args[1]));
                const int targetR = static_cast<int>(std::get<double>(args[2]));

                auto *move = reg->GetComponent<MovementComponent>(id);
                const auto *trans = reg->GetComponent<TransformComponent>(id);
                if (!move || !trans) return false;

                const MapComponent *map = nullptr;
                reg->ForEach<MapComponent>([&](Entity, const MapComponent *m) { map = m; });
                if (!map) return false;

                glm::vec2 pPos = trans->transform.GetPosition();
                const HexCoords startHex = move->isMoving && move->currentPathIndex < move->currentPath.size()
                                               ? move->currentPath[move->currentPathIndex]
                                               : Math::HexMath::PixelToHex({pPos.x, pPos.y});

                const HexCoords targetHex{targetQ, targetR};
                map->grid.FindPath(startHex, targetHex, move->currentPath);

                if (!move->currentPath.empty()) {
                    reg->ForEach<MapStateComponent>(
                        [&](Entity, MapStateComponent *state) {
                            state->pathTo = targetHex;
                            state->hasPathTo = true;
                        });
                    const Entity entity(id, reg);
                    MovementSystem::StartPath(entity);
                    return true;
                }
                return false;
            }, "SetPathToHex"));

    interpreter.get_global_environment()->define(
        "ClearSelectionOverlay", interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [reg = m_registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                reg->ForEach<MapStateComponent>([&](Entity, MapStateComponent *stateComp) {
                    stateComp->hasSelection = false;
                });
                return std::monostate{};
            }, "ClearSelectionOverlay"));

    interpreter.get_global_environment()->define(
        "ClearPathTarget", interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [reg = m_registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                reg->ForEach<MapStateComponent>([&](Entity, MapStateComponent *stateComp) {
                    stateComp->hasPathTo = false;
                });
                return std::monostate{};
            }, "ClearPathTarget"));
}
