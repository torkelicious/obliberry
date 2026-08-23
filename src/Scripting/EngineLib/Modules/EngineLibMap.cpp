#include "Scripting/EngineLib/EngineLib.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Systems/MovementSystem.h"
#include "Math/HexMath.h"
#include "Scripting/EngineLib/EngineLibFactories.h"
#include <ObSL/ScriptRuntime.h>
#include "Scripting/EngineLib/ScriptCommandBuffer.h"


//
// note: mapcomponent is treated as a singleton
//

void Scripting::EngineLib::register_map_modules(ObSL::Interpreter &interpreter) {
    // HEX MATH
    interpreter.get_global_environment()->define("Math_WorldToHex", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                            2,
                                                                            [](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                                                                auto *obj = interp->gc.allocate<ObSL::ObSLObject>();
                                                                                if (args.size() < 2 || !std::holds_alternative<double>(args[0]) || !std::holds_alternative<double>(args[1])) {
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
                                                                            },
                                                                            "Math_WorldToHex"));

    // SELECTION CONFIGURATION
    interpreter.get_global_environment()->define("GetSelectedHex", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                           0,
                                                                           [reg = m_registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
                                                                               std::shared_lock lock(g_RegistryMutex);
                                                                               auto *obj = interp->gc.allocate<ObSL::ObSLObject>();
                                                                               obj->fields["hasSelection"] = false;
                                                                               obj->fields["q"] = 0.0;
                                                                               obj->fields["r"] = 0.0;
                                                                               if (const auto *state = reg->GetFirst<ECS::Components::MapStateComponent>()) {
                                                                                   if (state->hasSelection) {
                                                                                       obj->fields["hasSelection"] = true;
                                                                                       obj->fields["q"] = static_cast<double>(state->selectedHex.q);
                                                                                       obj->fields["r"] = static_cast<double>(state->selectedHex.r);
                                                                                   }
                                                                               }
                                                                               return obj;
                                                                           },
                                                                           "GetSelectedHex"));

    interpreter.get_global_environment()->define("SetSelectedHex", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                           2,
                                                                           [reg = m_registry](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                                                               if (args.size() < 2 || !std::holds_alternative<double>(args[0]) || !std::holds_alternative<double>(args[1])) {
                                                                                   return std::monostate{};
                                                                               }

                                                                               const int q = static_cast<int>(std::get<double>(args[0]));
                                                                               const int r = static_cast<int>(std::get<double>(args[1]));
                                                                               auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
                                                                               auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
                                                                               if (cmd_buf) {
                                                                                   cmd_buf->push([q, r](ECS::Registry &reg) {
                                                                                       const Map::HexCoords targetHex{q, r};
                                                                                       const auto *map = reg.GetFirst<ECS::Components::MapComponent>();
                                                                                       auto *state = reg.GetFirst<ECS::Components::MapStateComponent>();
                                                                                       const Map::Tile *tile = map ? map->grid.Get(targetHex) : nullptr;
                                                                                       const bool isValid = tile && tile->walkable;
                                                                                       if (state) {
                                                                                           if (isValid) {
                                                                                               state->selectedHex = targetHex;
                                                                                               state->hasSelection = true;
                                                                                           } else {
                                                                                               state->hasSelection = false;
                                                                                           }
                                                                                       }
                                                                                   });
                                                                               } else if (reg) {
                                                                                   std::unique_lock lock(g_RegistryMutex);
                                                                                   const Map::HexCoords targetHex{q, r};
                                                                                   const auto *map = reg->GetFirst<ECS::Components::MapComponent>();
                                                                                   auto *state = reg->GetFirst<ECS::Components::MapStateComponent>();
                                                                                   const Map::Tile *tile = map ? map->grid.Get(targetHex) : nullptr;
                                                                                   const bool isValid = tile && tile->walkable;
                                                                                   if (state) {
                                                                                       if (isValid) {
                                                                                           state->selectedHex = targetHex;
                                                                                           state->hasSelection = true;
                                                                                       } else {
                                                                                           state->hasSelection = false;
                                                                                       }
                                                                                   }
                                                                               }
                                                                               return std::monostate{};
                                                                           },
                                                                           "SetSelectedHex"));


    // PATHFINDING & OVERLAYS
    interpreter.get_global_environment()->define("SetPathToHex",
                                                 interpreter.gc.allocate<ObSL::NativeFunction>(
                                                         3,
                                                         [reg = m_registry](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                                             if (args.size() < 3 || !std::holds_alternative<double>(args[0]) || !std::holds_alternative<double>(args[1]) || !std::holds_alternative<double>(args[2])) {
                                                                 return false;
                                                             }

                                                             const auto id = static_cast<ECS::EntityID>(std::get<double>(args[0]));
                                                             const int q = static_cast<int>(std::get<double>(args[1]));
                                                             const int r = static_cast<int>(std::get<double>(args[2]));

                                                             auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
                                                             auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
                                                             if (cmd_buf) {
                                                                 cmd_buf->push([id, q, r](ECS::Registry &reg) {
                                                                     const Map::HexCoords targetHex{q, r};

                                                                     auto *move = reg.GetComponent<ECS::Components::MovementComponent>(id);
                                                                     const auto *trans = reg.GetComponent<ECS::Components::TransformComponent>(id);
                                                                     if (!move || !trans)
                                                                         return;

                                                                     const auto *mapComp = reg.GetFirst<ECS::Components::MapComponent>();
                                                                     if (!mapComp)
                                                                         return;

                                                                     const Map::HexCoords startHex = Math::HexMath::PixelToHex({trans->transform.GetPosition().x, trans->transform.GetPosition().y});

                                                                     move->currentPath.clear();
                                                                     mapComp->grid.FindPath(startHex, targetHex, move->currentPath);
                                                                     move->currentPathIndex = 0;

                                                                     if (!move->currentPath.empty()) {
                                                                         if (auto *state = reg.GetFirst<ECS::Components::MapStateComponent>()) {
                                                                             state->pathTo = targetHex;
                                                                             state->hasPathTo = true;
                                                                         }
                                                                         const ECS::Entity entity(id, &reg);
                                                                         ECS::Systems::MovementSystem::StartPath(entity);
                                                                     }
                                                                 });
                                                             } else if (reg) {
                                                                 std::unique_lock lock(g_RegistryMutex);
                                                                 const Map::HexCoords targetHex{q, r};

                                                                 auto *move = reg->GetComponent<ECS::Components::MovementComponent>(id);
                                                                 const auto *trans = reg->GetComponent<ECS::Components::TransformComponent>(id);
                                                                 if (!move || !trans)
                                                                     return true;

                                                                 const auto *mapComp = reg->GetFirst<ECS::Components::MapComponent>();
                                                                 if (!mapComp)
                                                                     return true;

                                                                 const Map::HexCoords startHex = Math::HexMath::PixelToHex({trans->transform.GetPosition().x, trans->transform.GetPosition().y});

                                                                 move->currentPath.clear();
                                                                 mapComp->grid.FindPath(startHex, targetHex, move->currentPath);
                                                                 move->currentPathIndex = 0;

                                                                 if (!move->currentPath.empty()) {
                                                                     if (auto *state = reg->GetFirst<ECS::Components::MapStateComponent>()) {
                                                                         state->pathTo = targetHex;
                                                                         state->hasPathTo = true;
                                                                     }
                                                                     const ECS::Entity entity(id, reg);
                                                                     ECS::Systems::MovementSystem::StartPath(entity);
                                                                 }
                                                             }
                                                             return true;
                                                         },
                                                         "SetPathToHex"));

    interpreter.get_global_environment()->define("ClearSelectionOverlay", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                                  0,
                                                                                  [reg = m_registry](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &) -> ObSL::Value {
                                                                                      auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
                                                                                      auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
                                                                                      if (cmd_buf) {
                                                                                          cmd_buf->push([](ECS::Registry &reg) {
                                                                                              if (auto *stateComp = reg.GetFirst<ECS::Components::MapStateComponent>())
                                                                                                  stateComp->hasSelection = false;
                                                                                          });
                                                                                      } else if (reg) {
                                                                                          std::unique_lock lock(g_RegistryMutex);
                                                                                          if (auto *stateComp = reg->GetFirst<ECS::Components::MapStateComponent>())
                                                                                              stateComp->hasSelection = false;
                                                                                      }
                                                                                      return std::monostate{};
                                                                                  },
                                                                                  "ClearSelectionOverlay"));

    interpreter.get_global_environment()->define("ClearPathTarget", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                            0,
                                                                            [reg = m_registry](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &) -> ObSL::Value {
                                                                                auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
                                                                                auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
                                                                                if (cmd_buf) {
                                                                                    cmd_buf->push([](ECS::Registry &reg) {
                                                                                        if (auto *stateComp = reg.GetFirst<ECS::Components::MapStateComponent>())
                                                                                            stateComp->hasPathTo = false;
                                                                                    });
                                                                                } else if (reg) {
                                                                                    std::unique_lock lock(g_RegistryMutex);
                                                                                    if (auto *stateComp = reg->GetFirst<ECS::Components::MapStateComponent>())
                                                                                        stateComp->hasPathTo = false;
                                                                                }
                                                                                return std::monostate{};
                                                                            },
                                                                            "ClearPathTarget"));

    interpreter.get_global_environment()->define("Map_IsHexWalkable", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                              2,
                                                                              [reg = m_registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                                                                  if (args.size() < 2 || !std::holds_alternative<double>(args[0]) || !std::holds_alternative<double>(args[1])) {
                                                                                      return false;
                                                                                  }
                                                                                  std::shared_lock lock(g_RegistryMutex);
                                                                                  const Map::HexCoords pos(static_cast<int32_t>(std::get<double>(args[0])), static_cast<int32_t>(std::get<double>(args[1])));
                                                                                  const auto *mapComp = reg->GetFirst<ECS::Components::MapComponent>();
                                                                                  if (mapComp)
                                                                                      if (const Map::Tile *tile = mapComp->grid.Get(pos))
                                                                                          return tile->walkable;
                                                                                  return false;
                                                                              },
                                                                              "Map_IsHexWalkable"));

    interpreter.get_global_environment()->define("Map_GetMapEntity", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                             0,
                                                                             [reg = m_registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
                                                                                 std::shared_lock lock(g_RegistryMutex);
                                                                                 const ECS::EntityID mapId = reg->FindFirstEntity<ECS::Components::MapComponent>();
                                                                                 if (mapId != ECS::INVALID_ENTITY_ID) {
                                                                                     return CreateEntityObject(interp, *reg, mapId);
                                                                                 }
                                                                                 return std::monostate{};
                                                                             },
                                                                             "Map_GetMapEntity"));

    // Hex helpers
    interpreter.get_global_environment()->define("Hex_Distance", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                         4,
                                                                         [](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                                                             if (args.size() < 4 || !std::holds_alternative<double>(args[0]) || !std::holds_alternative<double>(args[1]) ||
                                                                                 !std::holds_alternative<double>(args[2]) || !std::holds_alternative<double>(args[3]))
                                                                                 return 0.0;
                                                                             const Map::HexCoords a(static_cast<int32_t>(std::get<double>(args[0])), static_cast<int32_t>(std::get<double>(args[1])));
                                                                             const Map::HexCoords b(static_cast<int32_t>(std::get<double>(args[2])), static_cast<int32_t>(std::get<double>(args[3])));
                                                                             return static_cast<double>(Math::HexMath::Distance(a, b));
                                                                         },
                                                                         "Hex_Distance"));

    interpreter.get_global_environment()->define("Hex_GetNeighbors", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                             2,
                                                                             [](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                                                                 if (args.size() < 2 || !std::holds_alternative<double>(args[0]) || !std::holds_alternative<double>(args[1]))
                                                                                     return std::monostate{};
                                                                                 const Map::HexCoords hex(static_cast<int32_t>(std::get<double>(args[0])), static_cast<int32_t>(std::get<double>(args[1])));
                                                                                 const auto neighbors = Math::HexMath::GetNeighbors(hex);
                                                                                 auto *arr = interp->gc.allocate<ObSL::ObSLArray>();
                                                                                 for (const auto &n : neighbors) {
                                                                                     auto *obj = interp->gc.allocate<ObSL::ObSLObject>();
                                                                                     obj->fields["q"] = static_cast<double>(n.q);
                                                                                     obj->fields["r"] = static_cast<double>(n.r);
                                                                                     arr->elements.emplace_back(obj);
                                                                                 }
                                                                                 return arr;
                                                                             },
                                                                             "Hex_GetNeighbors"));

    interpreter.get_global_environment()->define("Hex_HexToWorld", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                           2,
                                                                           [](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                                                               auto *obj = interp->gc.allocate<ObSL::ObSLObject>();
                                                                               if (args.size() < 2 || !std::holds_alternative<double>(args[0]) || !std::holds_alternative<double>(args[1])) {
                                                                                   obj->fields["x"] = 0.0;
                                                                                   obj->fields["y"] = 0.0;
                                                                                   return obj;
                                                                               }
                                                                               const Map::HexCoords hex(static_cast<int32_t>(std::get<double>(args[0])), static_cast<int32_t>(std::get<double>(args[1])));
                                                                               const glm::vec2 world = Math::HexMath::HexToWorld(hex);
                                                                               obj->fields["x"] = static_cast<double>(world.x);
                                                                               obj->fields["y"] = static_cast<double>(world.y);
                                                                               return obj;
                                                                           },
                                                                           "Hex_HexToWorld"));

    // muts
    interpreter.get_global_environment()->define("Map_SetHexWalkable", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                               2, // obj , bool
                                                                               [reg = m_registry](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                                                                   if (args.size() < 2 || !std::holds_alternative<ObSL::ObSLObject *>(args[0]) || !std::holds_alternative<bool>(args[1])) {
                                                                                       return std::monostate{};
                                                                                   }

                                                                                   const auto *obj = std::get<ObSL::ObSLObject *>(args[0]);
                                                                                   const bool walkable = std::get<bool>(args[1]);

                                                                                   if (!obj->fields.contains("q") || !obj->fields.contains("r")) {
                                                                                       return std::monostate{};
                                                                                   }

                                                                                   const auto &qVal = obj->fields.at("q");
                                                                                   const auto &rVal = obj->fields.at("r");

                                                                                   if (!std::holds_alternative<double>(qVal) || !std::holds_alternative<double>(rVal)) {
                                                                                       return std::monostate{};
                                                                                   }

                                                                                   const int32_t q = static_cast<int32_t>(std::get<double>(qVal));
                                                                                   const int32_t r = static_cast<int32_t>(std::get<double>(rVal));
                                                                                   const Map::HexCoords coords{q, r};

                                                                                   auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
                                                                                   auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
                                                                                   if (cmd_buf) {
                                                                                       cmd_buf->push([coords, walkable](ECS::Registry &reg) {
                                                                                           if (auto *mapComp = reg.GetFirst<ECS::Components::MapComponent>()) {
                                                                                               if (auto *tile = mapComp->grid.Get(coords)) {
                                                                                                   tile->walkable = walkable;
                                                                                                   mapComp->grid.SyncTileWalkableCache(coords);
                                                                                               }
                                                                                           }
                                                                                       });
                                                                                   } else if (reg) {
                                                                                       std::unique_lock lock(g_RegistryMutex);
                                                                                       if (auto *mapComp = reg->GetFirst<ECS::Components::MapComponent>()) {
                                                                                           if (auto *tile = mapComp->grid.Get(coords)) {
                                                                                               tile->walkable = walkable;
                                                                                               mapComp->grid.SyncTileWalkableCache(coords);
                                                                                           }
                                                                                       }
                                                                                   }

                                                                                   return std::monostate{};
                                                                               },
                                                                               "Map_SetHexWalkable"));


    interpreter.get_global_environment()->define("Map_SetTileType", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                            2, // obj ,num
                                                                            [reg = m_registry](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                                                                if (args.size() < 2 || !std::holds_alternative<ObSL::ObSLObject *>(args[0]) || !std::holds_alternative<double>(args[1])) {
                                                                                    return std::monostate{};
                                                                                }

                                                                                const auto *obj = std::get<ObSL::ObSLObject *>(args[0]);
                                                                                const double type = std::get<double>(args[1]);

                                                                                if (!obj->fields.contains("q") || !obj->fields.contains("r")) {
                                                                                    return std::monostate{};
                                                                                }

                                                                                const auto &qVal = obj->fields.at("q");
                                                                                const auto &rVal = obj->fields.at("r");

                                                                                if (!std::holds_alternative<double>(qVal) || !std::holds_alternative<double>(rVal)) {
                                                                                    return std::monostate{};
                                                                                }

                                                                                const int32_t q = static_cast<int32_t>(std::get<double>(qVal));
                                                                                const int32_t r = static_cast<int32_t>(std::get<double>(rVal));
                                                                                const Map::HexCoords coords{q, r};

                                                                                auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
                                                                                auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
                                                                                if (cmd_buf) {
                                                                                    cmd_buf->push([coords, type](ECS::Registry &reg) {
                                                                                        if (auto *mapComp = reg.GetFirst<ECS::Components::MapComponent>()) {
                                                                                            if (auto *tile = mapComp->grid.Get(coords)) {
                                                                                                tile->type = static_cast<uint8_t>(type);
                                                                                            }
                                                                                        }
                                                                                    });
                                                                                } else if (reg) {
                                                                                    std::unique_lock lock(g_RegistryMutex);
                                                                                    if (auto *mapComp = reg->GetFirst<ECS::Components::MapComponent>()) {
                                                                                        if (auto *tile = mapComp->grid.Get(coords)) {
                                                                                            tile->type = static_cast<uint8_t>(type);
                                                                                        }
                                                                                    }
                                                                                }

                                                                                return std::monostate{};
                                                                            },
                                                                            "Map_SetTileType"));
}
