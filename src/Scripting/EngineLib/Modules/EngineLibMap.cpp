#include "../EngineLib.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Systems/MovementSystem.h"
#include "Math/HexMath.h"
#include "Scripting/ObSLCore/Interpreter/Interpreter.h"

void Scripting::EngineLib::EngineLib::register_map_modules(ObSL::Interpreter &interpreter) {
    // HEX MATH
    interpreter.get_global_environment()->define(
        "Math_WorldToHex", interpreter.gc.allocate<ObSL::NativeFunction>(
            2,
            [](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                auto *obj = interp->gc.allocate<ObSL::ObSLObject>();
                if (args.size() < 2 || !std::holds_alternative<double>(args[0]) || !std::holds_alternative<
                        double>(args[1])) {
                    obj->fields["q"] = 0.0;
                    obj->fields["r"] = 0.0;
                    return obj;
                }

                auto x = static_cast<float>(std::get<double>(args[0]));
                auto y = static_cast<float>(std::get<double>(args[1]));

                const Map::HexCoords hex = Math::HexMath::PixelToHex({x, y});

                obj->fields["q"] = static_cast<double>(hex.q);
                obj->fields["r"] = static_cast<double>(hex.r);
                return obj;
            }, "Math_WorldToHex"));

    // SELECTION CONFIGURATION
    interpreter.get_global_environment()->define(
        "GetSelectedHex", interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [reg = m_registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
                auto *obj = interp->gc.allocate<ObSL::ObSLObject>();
                obj->fields["hasSelection"] = false;
                obj->fields["q"] = 0.0;
                obj->fields["r"] = 0.0;
                reg->ForEach<ECS::Components::MapStateComponent>(
                    [&](ECS::Entity, const ECS::Components::MapStateComponent *state) {
                        if (state->hasSelection) {
                            obj->fields["hasSelection"] = true;
                            obj->fields["q"] = static_cast<double>(state->selectedHex.q);
                            obj->fields["r"] = static_cast<double>(state->selectedHex.r);
                        }
                    });
                return obj;
            }, "GetSelectedHex"));

    interpreter.get_global_environment()->define(
        "SetSelectedHex", interpreter.gc.allocate<ObSL::NativeFunction>(
            2,
            [reg = m_registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (args.size() < 2 || !std::holds_alternative<double>(args[0]) || !std::holds_alternative<
                        double>(args[1])) {
                    return std::monostate{};
                }

                const int q = static_cast<int>(std::get<double>(args[0]));
                const int r = static_cast<int>(std::get<double>(args[1]));
                const Map::HexCoords targetHex{q, r};
                bool isValid = false;
                reg->ForEach<ECS::Components::MapComponent>([&](ECS::Entity, ECS::Components::MapComponent *map) {
                    if (const auto *tile = map->grid.Get(targetHex); tile && tile->walkable) {
                        isValid = true;
                    }
                });

                reg->ForEach<ECS::Components::MapStateComponent>(
                    [&](ECS::Entity, ECS::Components::MapStateComponent *stateComp) {
                        if (isValid) {
                            stateComp->selectedHex = targetHex;
                            stateComp->hasSelection = true;
                        } else {
                            stateComp->hasSelection = false;
                        }
                    });
                return std::monostate{};
            }, "SetSelectedHex"));


    // PATHFINDING & OVERLAYS
    interpreter.get_global_environment()->define(
        "SetPathToHex", interpreter.gc.allocate<ObSL::NativeFunction>(
            3,
            [reg = m_registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (args.size() < 3 ||
                    !std::holds_alternative<double>(args[0]) ||
                    !std::holds_alternative<double>(args[1]) ||
                    !std::holds_alternative<double>(args[2])) {
                    return false;
                }

                const auto id = static_cast<ECS::EntityID>(std::get<double>(args[0]));
                const int q = static_cast<int>(std::get<double>(args[1]));
                const int r = static_cast<int>(std::get<double>(args[2]));
                const Map::HexCoords targetHex{q, r};

                auto *move = reg->GetComponent<ECS::Components::MovementComponent>(id);
                const auto *trans = reg->GetComponent<ECS::Components::TransformComponent>(id);
                if (!move || !trans) return false;

                const ECS::Components::MapComponent *mapComp = nullptr;
                reg->ForEach<ECS::Components::MapComponent>([&](ECS::Entity, const ECS::Components::MapComponent *map) {
                    mapComp = map;
                });

                if (!mapComp) return false;

                const Map::HexCoords startHex = Math::HexMath::PixelToHex({
                    trans->transform.GetPosition().x, trans->transform.GetPosition().y
                });

                move->currentPath.clear();
                mapComp->grid.FindPath(startHex, targetHex, move->currentPath);
                move->currentPathIndex = 0;

                if (!move->currentPath.empty()) {
                    reg->ForEach<ECS::Components::MapStateComponent>(
                        [&](ECS::Entity, ECS::Components::MapStateComponent *state) {
                            state->pathTo = targetHex;
                            state->hasPathTo = true;
                        });
                    const ECS::Entity entity(id, reg);
                    ECS::Systems::MovementSystem::StartPath(entity);
                    return true;
                }
                return false;
            }, "SetPathToHex"));

    interpreter.get_global_environment()->define(
        "ClearSelectionOverlay", interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [reg = m_registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                reg->ForEach<ECS::Components::MapStateComponent>(
                    [&](ECS::Entity, ECS::Components::MapStateComponent *stateComp) {
                        stateComp->hasSelection = false;
                    });
                return std::monostate{};
            }, "ClearSelectionOverlay"));

    interpreter.get_global_environment()->define(
        "ClearPathTarget", interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [reg = m_registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                reg->ForEach<ECS::Components::MapStateComponent>(
                    [&](ECS::Entity, ECS::Components::MapStateComponent *stateComp) {
                        stateComp->hasPathTo = false;
                    });
                return std::monostate{};
            }, "ClearPathTarget"));
}
