#pragma once

#include "ECS/Registry.h"
#include "Scripting/ObSLCore/Interpreter/Interpreter.h"

#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/PointLightComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Components/DirectionalTextureComponent.h"
#include "ECS/Components/BillboardTagComponent.h"
#include "ECS/Components/DestroyTagComponent.h"


// dec so other files can call it
ObSL::ObSLObject *CreateEntityObject(ObSL::Interpreter *interpreter, Registry &registry, EntityID id);

namespace EngineLibFactories {
    // GC Guard
    struct GCProtectGuard {
        ObSL::Interpreter *interpreter;

        GCProtectGuard(ObSL::Interpreter *interp, const ObSL::Value &val) : interpreter(interp) {
            interpreter->gc_protect_stack.emplace_back(val);
        }

        ~GCProtectGuard() {
            interpreter->gc_protect_stack.pop_back();
        }

        GCProtectGuard(const GCProtectGuard &) = delete;

        GCProtectGuard &operator=(const GCProtectGuard &) = delete;
    };

    // TRANSFORM COMPONENT
    inline ObSL::ObSLObject *CreateTransformObject(ObSL::Interpreter *interpreter, Registry &registry, EntityID id) {
        auto *obj = interpreter->gc.allocate<ObSL::ObSLObject>();
        GCProtectGuard guard(interpreter, obj);

        auto set_pos = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.size() >= 3 && std::holds_alternative<double>(args[0]) && std::holds_alternative<double>(args[1])
                && std::holds_alternative<double>(args[2])) {
                if (auto *comp = registry.GetComponent<TransformComponent>(id)) {
                    comp->transform.SetPosition({
                        static_cast<float>(std::get<double>(args[0])),
                        static_cast<float>(std::get<double>(args[1])),
                        static_cast<float>(std::get<double>(args[2]))
                    });
                }
            }
            return std::monostate{};
        };

        auto set_rot = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.size() >= 3 && std::holds_alternative<double>(args[0]) && std::holds_alternative<double>(args[1])
                && std::holds_alternative<double>(args[2])) {
                if (auto *comp = registry.GetComponent<TransformComponent>(id)) {
                    comp->transform.SetRotation({
                        static_cast<float>(std::get<double>(args[0])),
                        static_cast<float>(std::get<double>(args[1])),
                        static_cast<float>(std::get<double>(args[2]))
                    });
                }
            }
            return std::monostate{};
        };

        auto set_scale = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.size() >= 3 && std::holds_alternative<double>(args[0]) && std::holds_alternative<double>(args[1])
                && std::holds_alternative<double>(args[2])) {
                if (auto *comp = registry.GetComponent<TransformComponent>(id)) {
                    comp->transform.SetScale({
                        static_cast<float>(std::get<double>(args[0])),
                        static_cast<float>(std::get<double>(args[1])),
                        static_cast<float>(std::get<double>(args[2]))
                    });
                }
            }
            return std::monostate{};
        };

        auto get_pos = [id, &registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
            if (const auto *comp = registry.GetComponent<TransformComponent>(id)) {
                auto *arr = interp->gc.allocate<ObSL::ObSLArray>();
                const auto &pos = comp->transform.GetPosition();
                arr->elements.emplace_back(pos.x);
                arr->elements.emplace_back(pos.y);
                arr->elements.emplace_back(pos.z);
                return arr;
            }
            return std::monostate{};
        };

        auto get_rot = [id, &registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
            if (const auto *comp = registry.GetComponent<TransformComponent>(id)) {
                auto *arr = interp->gc.allocate<ObSL::ObSLArray>();
                const auto &rot = comp->transform.GetRotation();
                arr->elements.emplace_back(rot.x);
                arr->elements.emplace_back(rot.y);
                arr->elements.emplace_back(rot.z);
                return arr;
            }
            return std::monostate{};
        };

        auto get_scale = [id, &registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
            if (const auto *comp = registry.GetComponent<TransformComponent>(id)) {
                auto *arr = interp->gc.allocate<ObSL::ObSLArray>();
                const auto &scale = comp->transform.GetScale();
                arr->elements.emplace_back(scale.x);
                arr->elements.emplace_back(scale.y);
                arr->elements.emplace_back(scale.z);
                return arr;
            }
            return std::monostate{};
        };

        auto is_moving_body = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
            if (auto *move = registry.GetComponent<MovementComponent>(id)) {
                return move->isMoving;
            }
            return false;
        };

        obj->fields["SetPosition"] = interpreter->gc.allocate<ObSL::NativeFunction>(
            3, std::move(set_pos), "SetPosition");
        obj->fields["SetRotation"] = interpreter->gc.allocate<ObSL::NativeFunction>(
            3, std::move(set_rot), "SetRotation");
        obj->fields["SetScale"] = interpreter->gc.allocate<ObSL::NativeFunction>(3, std::move(set_scale), "SetScale");
        obj->fields["GetPosition"] = interpreter->gc.allocate<ObSL::NativeFunction>(
            0, std::move(get_pos), "GetPosition");
        obj->fields["GetRotation"] = interpreter->gc.allocate<ObSL::NativeFunction>(
            0, std::move(get_rot), "GetRotation");
        obj->fields["GetScale"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(get_scale), "GetScale");
        obj->fields["IsMoving"] = interpreter->gc.allocate<ObSL::NativeFunction>(
            0, std::move(is_moving_body), "IsMoving");

        return obj;
    }

    // POINT LIGHT COMPONENT
    inline ObSL::ObSLObject *CreatePointLightObject(ObSL::Interpreter *interpreter, Registry &registry, EntityID id) {
        auto *obj = interpreter->gc.allocate<ObSL::ObSLObject>();
        GCProtectGuard guard(interpreter, obj);

        auto set_color = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.size() >= 3 && std::holds_alternative<double>(args[0]) && std::holds_alternative<double>(args[1])
                && std::holds_alternative<double>(args[2])) {
                if (auto *comp = registry.GetComponent<PointLightComponent>(id)) {
                    comp->color.r = static_cast<float>(std::get<double>(args[0]));
                    comp->color.g = static_cast<float>(std::get<double>(args[1]));
                    comp->color.b = static_cast<float>(std::get<double>(args[2]));
                }
            }
            return std::monostate{};
        };

        auto set_intensity = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (!args.empty() && std::holds_alternative<double>(args[0])) {
                if (auto *comp = registry.GetComponent<PointLightComponent>(id))
                    comp->intensity = static_cast<float>(std::get<double>(args[0]));
            }
            return std::monostate{};
        };

        auto set_radius = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (!args.empty() && std::holds_alternative<double>(args[0])) {
                if (auto *comp = registry.GetComponent<PointLightComponent>(id))
                    comp->radius = static_cast<float>(std::get<double>(args[0]));
            }
            return std::monostate{};
        };

        obj->fields["SetColor"] = interpreter->gc.allocate<ObSL::NativeFunction>(3, std::move(set_color), "SetColor");
        obj->fields["SetIntensity"] = interpreter->gc.allocate<ObSL::NativeFunction>(
            1, std::move(set_intensity), "SetIntensity");
        obj->fields["SetRadius"] = interpreter->gc.allocate<
            ObSL::NativeFunction>(1, std::move(set_radius), "SetRadius");

        return obj;
    }

    // MOVEMENT COMPONENT
    inline ObSL::ObSLObject *CreateMovementObject(ObSL::Interpreter *interpreter, Registry &registry, EntityID id) {
        auto *obj = interpreter->gc.allocate<ObSL::ObSLObject>();
        GCProtectGuard guard(interpreter, obj);

        auto get_is_moving = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
            if (auto *comp = registry.GetComponent<MovementComponent>(id)) return comp->isMoving;
            return false;
        };

        auto set_is_moving = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (!args.empty() && std::holds_alternative<bool>(args[0])) {
                if (auto *comp = registry.GetComponent<MovementComponent>(id)) comp->isMoving = std::get<bool>(args[0]);
            }
            return std::monostate{};
        };

        auto set_time_per_step = [id, &registry](ObSL::Interpreter *,
                                                 const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (!args.empty() && std::holds_alternative<double>(args[0])) {
                if (auto *comp = registry.GetComponent<MovementComponent>(id))
                    comp->timePerStep = static_cast<float>(std::get<double>(args[0]));
            }
            return std::monostate{};
        };

        obj->fields["GetIsMoving"] = interpreter->gc.allocate<ObSL::NativeFunction>(
            0, std::move(get_is_moving), "GetIsMoving");
        obj->fields["SetIsMoving"] = interpreter->gc.allocate<ObSL::NativeFunction>(
            1, std::move(set_is_moving), "SetIsMoving");
        obj->fields["SetTimePerStep"] = interpreter->gc.allocate<ObSL::NativeFunction>(
            1, std::move(set_time_per_step), "SetTimePerStep");

        return obj;
    }

    // MAP STATE COMPONENT
    inline ObSL::ObSLObject *CreateMapStateObject(ObSL::Interpreter *interpreter, Registry &registry, EntityID id) {
        auto *obj = interpreter->gc.allocate<ObSL::ObSLObject>();
        GCProtectGuard guard(interpreter, obj);

        auto get_has_selection = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
            if (auto *comp = registry.GetComponent<MapStateComponent>(id)) return comp->hasSelection;
            return false;
        };

        auto get_selected_hex = [id, &registry](ObSL::Interpreter *interp,
                                                const std::vector<ObSL::Value> &) -> ObSL::Value {
            if (const auto *comp = registry.GetComponent<MapStateComponent>(id)) {
                auto *arr = interp->gc.allocate<ObSL::ObSLArray>();
                arr->elements.emplace_back(static_cast<double>(comp->selectedHex.q));
                arr->elements.emplace_back(static_cast<double>(comp->selectedHex.r));
                return arr;
            }
            return std::monostate{};
        };

        auto get_path_to_hex = [id, &registry](ObSL::Interpreter *interp,
                                               const std::vector<ObSL::Value> &) -> ObSL::Value {
            if (const auto *comp = registry.GetComponent<MapStateComponent>(id)) {
                auto *arr = interp->gc.allocate<ObSL::ObSLArray>();
                arr->elements.emplace_back(static_cast<double>(comp->pathTo.q));
                arr->elements.emplace_back(static_cast<double>(comp->pathTo.r));
                return arr;
            }
            return std::monostate{};
        };

        obj->fields["GetHasSelection"] = interpreter->gc.allocate<ObSL::NativeFunction>(
            0, std::move(get_has_selection), "GetHasSelection");
        obj->fields["GetSelectedHex"] = interpreter->gc.allocate<ObSL::NativeFunction>(
            0, std::move(get_selected_hex), "GetSelectedHex");
        obj->fields["GetPathToHex"] = interpreter->gc.allocate<ObSL::NativeFunction>(
            0, std::move(get_path_to_hex), "GetPathToHex");

        return obj;
    }

    // DIRECTIONAL TEXTURE COMPONENT
    inline ObSL::ObSLObject *CreateDirectionalTextureObject(ObSL::Interpreter *interpreter, Registry &registry,
                                                            EntityID id) {
        auto *obj = interpreter->gc.allocate<ObSL::ObSLObject>();
        GCProtectGuard guard(interpreter, obj);

        auto set_index = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (!args.empty() && std::holds_alternative<double>(args[0])) {
                if (auto *comp = registry.GetComponent<DirectionalTextureComponent>(id))
                    comp->index = static_cast<int>(std::get<double>(args[0]));
            }
            return std::monostate{};
        };

        obj->fields["SetIndex"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(set_index), "SetIndex");
        return obj;
    }

    // TAG COMPONENTS
    inline ObSL::ObSLObject *CreateBillboardTagObject(ObSL::Interpreter *interpreter, Registry &, EntityID) {
        return interpreter->gc.allocate<ObSL::ObSLObject>();
    }

    inline ObSL::ObSLObject *CreateDestroyTagObject(ObSL::Interpreter *interpreter, Registry &, EntityID) {
        return interpreter->gc.allocate<ObSL::ObSLObject>();
    }
}
