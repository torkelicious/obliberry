#include "EngineLib.h"
#include "EngineLibFactories.h"
#include <string>

#include "Core/InputManager.h"
#include "Renderer/Camera.h"
#include "Core/Window.h"
#include "Scripting/ObSLCore/Interpreter/Interpreter.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Systems/MovementSystem.h"
#include "IO/PrefabManager.h"
#include "Math/HexMath.h"

// Entity Wrapper Object
ObSL::ObSLObject *CreateEntityObject(ObSL::Interpreter *interpreter, Registry &registry, EntityID id) {
    auto *obj = interpreter->gc.allocate<ObSL::ObSLObject>();
    EngineLibFactories::GCProtectGuard guard(interpreter, obj);

    obj->fields["id"] = static_cast<double>(id);
    obj->fields["name"] = registry.GetEntityName(id);

    // SetName
    auto set_name_body = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (!args.empty() && std::holds_alternative<std::string>(args[0])) {
            registry.SetEntityName(id, std::get<std::string>(args[0]));
        }
        return std::monostate{};
    };

    // GetComponent
    auto get_comp_body = [id, &registry
            ](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) return std::monostate{};
        const auto &comp_name = std::get<std::string>(args[0]);

#define GET_COMP(NameStr, CompType, Factory) \
            if (comp_name == NameStr && registry.HasComponent<CompType>(id)) \
                return EngineLibFactories::Factory(interp, registry, id)

        GET_COMP("Transform", TransformComponent, CreateTransformObject);
        GET_COMP("PointLight", PointLightComponent, CreatePointLightObject);
        GET_COMP("Movement", MovementComponent, CreateMovementObject);
        GET_COMP("MapState", MapStateComponent, CreateMapStateObject);
        GET_COMP("DirectionalTexture", DirectionalTextureComponent, CreateDirectionalTextureObject);
        GET_COMP("BillboardTag", BillboardTagComponent, CreateBillboardTagObject);
        GET_COMP("DestroyTag", DestroyTagComponent, CreateDestroyTagObject);

#undef GET_COMP
        return std::monostate{};
    };

    // HasComponent
    auto has_comp_body = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) return false;
        const auto &comp_name = std::get<std::string>(args[0]);

#define HAS_COMP(NameStr, CompType) \
            if (comp_name == NameStr) return registry.HasComponent<CompType>(id)

        HAS_COMP("Transform", TransformComponent);
        HAS_COMP("PointLight", PointLightComponent);
        HAS_COMP("Movement", MovementComponent);
        HAS_COMP("MapState", MapStateComponent);
        HAS_COMP("DirectionalTexture", DirectionalTextureComponent);
        HAS_COMP("BillboardTag", BillboardTagComponent);
        HAS_COMP("DestroyTag", DestroyTagComponent);

#undef HAS_COMP
        return false;
    };

    // AddComponent
    auto add_comp_body = [id, &registry
            ](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) return std::monostate{};
        const auto &comp_name = std::get<std::string>(args[0]);

#define ADD_COMP(NameStr, CompType, Factory) \
            if (comp_name == NameStr) { \
                if (!registry.HasComponent<CompType>(id)) registry.AddComponent<CompType>(id); \
                return EngineLibFactories::Factory(interp, registry, id); \
            }

        ADD_COMP("Transform", TransformComponent, CreateTransformObject);
        ADD_COMP("PointLight", PointLightComponent, CreatePointLightObject);
        ADD_COMP("Movement", MovementComponent, CreateMovementObject);
        ADD_COMP("MapState", MapStateComponent, CreateMapStateObject);
        ADD_COMP("DirectionalTexture", DirectionalTextureComponent, CreateDirectionalTextureObject);
        ADD_COMP("BillboardTag", BillboardTagComponent, CreateBillboardTagObject);
        ADD_COMP("DestroyTag", DestroyTagComponent, CreateDestroyTagObject);

#undef ADD_COMP
        return std::monostate{};
    };

    // RemoveComponent
    auto remove_comp_body = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) return std::monostate{};
        const auto &comp_name = std::get<std::string>(args[0]);

#define REMOVE_COMP(NameStr, CompType) \
            if (comp_name == NameStr) { registry.RemoveComponent<CompType>(id); return std::monostate{}; }

        REMOVE_COMP("Transform", TransformComponent);
        REMOVE_COMP("PointLight", PointLightComponent);
        REMOVE_COMP("Movement", MovementComponent);
        REMOVE_COMP("MapState", MapStateComponent);
        REMOVE_COMP("DirectionalTexture", DirectionalTextureComponent);
        REMOVE_COMP("BillboardTag", BillboardTagComponent);

#undef REMOVE_COMP
        return std::monostate{};
    };

    // Destroy Entity Tagging
    auto destroy_body = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
        if (!registry.HasComponent<DestroyTagComponent>(id)) registry.AddComponent<DestroyTagComponent>(id);
        return std::monostate{};
    };

    obj->fields["SetName"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(set_name_body), "SetName");
    obj->fields["GetComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(
        1, std::move(get_comp_body), "GetComponent");
    obj->fields["HasComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(
        1, std::move(has_comp_body), "HasComponent");
    obj->fields["AddComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(
        1, std::move(add_comp_body), "AddComponent");
    obj->fields["RemoveComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(
        1, std::move(remove_comp_body), "RemoveComponent");
    obj->fields["Destroy"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(destroy_body), "Destroy");

    return obj;
}

// Global Modules
void EngineLib::register_modules(ObSL::Interpreter &interpreter) {
    Registry *reg = m_registry;
    EngineContext *ctx = m_ctx;

    interpreter.get_global_environment()->define(
        "CloseWindow",
        interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                glfwSetWindowShouldClose(ctx->window->GetNativeWindow(), true);
                return std::monostate{};
            },
            "CloseWindow"
        ));

    // FIND & CREATE
    interpreter.get_global_environment()->define(
        "Find", interpreter.gc.allocate<ObSL::NativeFunction>(
            1,
            [reg](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (args.empty() || !std::holds_alternative<std::string>(args[0])) return std::monostate{};
                const auto target_name = std::get<std::string>(args[0]);
                for (const EntityID id: reg->GetLivingEntities()) {
                    if (reg->GetEntityName(id) == target_name)
                        return CreateEntityObject(interp, *reg, id);
                }
                return std::monostate{};
            }, "Find"));

    interpreter.get_global_environment()->define(
        "CreateEntity", interpreter.gc.allocate<ObSL::NativeFunction>(
            0, [reg](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                std::string name = "NewEntity";
                if (!args.empty() && std::holds_alternative<std::string>(args[0])) {
                    name = std::get<std::string>(args[0]);
                }
                const EntityID new_id = reg->CreateEntity();
                reg->SetEntityName(new_id, name);
                return CreateEntityObject(interp, *reg, new_id);
            }, "CreateEntity"));

    // TIME
    interpreter.get_global_environment()->define(
        "get_dt", interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                return ctx ? static_cast<double>(ctx->deltaTime) : 0.0;
            }, "get_dt"));

    // INPUT SUBSYSTEM
    interpreter.get_global_environment()->define(
        "Input_IsKeyDown", interpreter.gc.allocate<ObSL::NativeFunction>(
            1,
            [ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (!ctx || !ctx->input || args.empty() || !std::holds_alternative<std::string>(args[0])) return false;
                return ctx->input->IsKeyDown(InputManager::GetKeyFromName(std::get<std::string>(args[0])));
            }, "Input_IsKeyDown"));

    interpreter.get_global_environment()->define(
        "Input_IsKeyPressed", interpreter.gc.allocate<ObSL::NativeFunction>(
            1,
            [ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (!ctx || !ctx->input || args.empty() || !std::holds_alternative<std::string>(args[0])) return false;
                return ctx->input->IsKeyPressed(InputManager::GetKeyFromName(std::get<std::string>(args[0])));
            }, "Input_IsKeyPressed"));

    interpreter.get_global_environment()->define(
        "Input_IsMousePressed", interpreter.gc.allocate<ObSL::NativeFunction>(
            1,
            [ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (!ctx || !ctx->input || args.empty() || !std::holds_alternative<double>(args[0])) return false;
                return ctx->input->IsMousePressed(static_cast<int>(std::get<double>(args[0])));
            }, "Input_IsMousePressed"));

    interpreter.get_global_environment()->define(
        "Input_GetMouseWorldPos",
        interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
                auto *obj = interp->gc.allocate<ObSL::ObSLObject>();
                if (ctx && ctx->input && ctx->camera && ctx->window) {
                    const float w = static_cast<float>(ctx->window->GetWidth());
                    const float h = static_cast<float>(ctx->window->GetHeight());
                    const glm::vec2 m{
                        static_cast<float>(ctx->input->MousePosX()),
                        static_cast<float>(ctx->input->MousePosY())
                    };
                    const glm::vec2 world = ctx->camera->MouseToWorld(m.x, m.y, w, h);
                    obj->fields["x"] = static_cast<double>(world.x);
                    obj->fields["y"] = static_cast<double>(world.y);
                } else {
                    obj->fields["x"] = 0.0;
                    obj->fields["y"] = 0.0;
                }
                return obj;
            }, "Input_GetMouseWorldPos"));

    // MAP & PATHFINDING
    interpreter.get_global_environment()->define(
        "GetSelectedHex", interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [reg](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
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
            [reg](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (args.size() < 3 || !std::holds_alternative<double>(args[0])
                    || !std::holds_alternative<double>(args[1])
                    || !std::holds_alternative<double>(args[2]))
                    return false;

                const EntityID id = static_cast<EntityID>(std::get<double>(args[0]));
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

    // CAMERA CONTROL
    interpreter.get_global_environment()->define(
        "Camera_GetPosition", interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
                auto *obj = interp->gc.allocate<ObSL::ObSLObject>();
                if (ctx && ctx->camera) {
                    obj->fields["x"] = static_cast<double>(ctx->camera->Position.x);
                    obj->fields["y"] = static_cast<double>(ctx->camera->Position.y);
                    obj->fields["z"] = static_cast<double>(ctx->camera->Position.z);
                } else {
                    obj->fields["x"] = 0.0;
                    obj->fields["y"] = 0.0;
                    obj->fields["z"] = 0.0;
                }
                return obj;
            }, "Camera_GetPosition"));

    interpreter.get_global_environment()->define(
        "Camera_SetPosition", interpreter.gc.allocate<ObSL::NativeFunction>(
            3,
            [ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (ctx && ctx->camera && args.size() >= 3
                    && std::holds_alternative<double>(args[0])
                    && std::holds_alternative<double>(args[1])
                    && std::holds_alternative<double>(args[2])) {
                    ctx->camera->Position.x = static_cast<float>(std::get<double>(args[0]));
                    ctx->camera->Position.y = static_cast<float>(std::get<double>(args[1]));
                    ctx->camera->Position.z = static_cast<float>(std::get<double>(args[2]));
                    return true;
                }
                return false;
            }, "Camera_SetPosition"));

    interpreter.get_global_environment()->define(
        "Camera_Move", interpreter.gc.allocate<ObSL::NativeFunction>(
            3,
            [ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (ctx && ctx->camera && args.size() >= 3
                    && std::holds_alternative<double>(args[0])
                    && std::holds_alternative<double>(args[1])
                    && std::holds_alternative<double>(args[2])) {
                    ctx->camera->Position.x += static_cast<float>(std::get<double>(args[0]));
                    ctx->camera->Position.y += static_cast<float>(std::get<double>(args[1]));
                    ctx->camera->Position.z += static_cast<float>(std::get<double>(args[2]));
                    return true;
                }
                return false;
            }, "Camera_Move"));

    interpreter.get_global_environment()->define(
        "Camera_PanScreenSpace", interpreter.gc.allocate<ObSL::NativeFunction>(
            2,
            [ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (ctx && ctx->camera && args.size() >= 2
                    && std::holds_alternative<double>(args[0])
                    && std::holds_alternative<double>(args[1])) {
                    glm::vec2 screenPan(
                        static_cast<float>(std::get<double>(args[0])),
                        static_cast<float>(std::get<double>(args[1]))
                    );

                    constexpr float VERTICAL_COMPENSATION = 1.4f;
                    screenPan.y *= VERTICAL_COMPENSATION;

                    const glm::mat4 invRot = glm::inverse(ctx->camera->GetRotation());
                    const glm::vec4 worldPan = invRot * glm::vec4(screenPan.x, screenPan.y, 0.0f, 0.0f);

                    ctx->camera->Position += glm::vec3(worldPan.x, worldPan.y, 0.0f) * (1.0f / ctx->camera->Zoom);
                    return true;
                }
                return false;
            }, "Camera_PanScreenSpace"
        ));

    interpreter.get_global_environment()->define(
        "Camera_GetZoom", interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                if (ctx && ctx->camera) return ctx->camera->Zoom;
                return 1.0;
            }, "Camera_GetZoom"
        ));

    interpreter.get_global_environment()->define(
        "ClearSelectionOverlay",
        interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [reg = m_registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                reg->ForEach<MapStateComponent>([&](Entity, MapStateComponent *stateComp) {
                    stateComp->hasSelection = false;
                });
                return std::monostate{};
            },
            "ClearSelectionOverlay"
        ));

    interpreter.get_global_environment()->define(
        "ClearPathTarget",
        interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [reg = m_registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                reg->ForEach<MapStateComponent>([&](Entity, MapStateComponent *stateComp) {
                    stateComp->hasPathTo = false;
                });
                return std::monostate{};
            },
            "ClearPathTarget"
        ));

    interpreter.get_global_environment()->define(
        "Instantiate", interpreter.gc.allocate<ObSL::NativeFunction>(
            1, // the filepath
            [reg = m_registry, ctx = m_ctx](ObSL::Interpreter *interp,
                                            const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (args.empty() || !std::holds_alternative<std::string>(args[0])) return std::monostate{};
                std::string prefab_path = std::get<std::string>(args[0]);
                EntityID new_id = PrefabManager::Instantiate(*reg, *(ctx->resources), prefab_path);
                if (new_id == 0) return std::monostate{}; // Failed to load
                return CreateEntityObject(interp, *reg, new_id);
            }, "Instantiate"));
}
