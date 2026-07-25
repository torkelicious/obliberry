#pragma once

#include <mutex>
#include <shared_mutex>

#include "ECS/Registry.h"
#include <ObSL/Interpreter.h>
#include <ObSL/ScriptWorker.h>
#include "Scripting/EngineLib/ScriptCommandBuffer.h"

#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/PointLightComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Components/DirectionalTextureComponent.h"
#include "ECS/Components/ParticleEmitterComponent.h"


namespace Scripting {
    // Protects all ECS registry access during parallel script execution.
    inline std::shared_mutex g_RegistryMutex;

    // dec so other files can call it
    ObSL::ObSLObject *CreateEntityObject(ObSL::Interpreter *interpreter, ECS::Registry &registry, ECS::EntityID id);

    namespace EngineLibFactories {
        // GC Guard
        struct GCProtectGuard {
            ObSL::Interpreter *interpreter;

            GCProtectGuard(ObSL::Interpreter *interp, const ObSL::Value &val) : interpreter(interp) { interpreter->gc_protect_stack.emplace_back(val); }

            ~GCProtectGuard() { interpreter->gc_protect_stack.pop_back(); }

            GCProtectGuard(const GCProtectGuard &) = delete;

            GCProtectGuard &operator=(const GCProtectGuard &) = delete;
        };

        // TRANSFORM COMPONENT
        inline ObSL::ObSLObject *CreateTransformObject(ObSL::Interpreter *interpreter, ECS::Registry &registry, ECS::EntityID id) {
            auto *obj = interpreter->gc.allocate<ObSL::ObSLObject>();
            GCProtectGuard guard(interpreter, obj);

            auto set_pos = [id, reg_ptr = &registry](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (args.size() >= 3 && std::holds_alternative<double>(args[0]) && std::holds_alternative<double>(args[1]) && std::holds_alternative<double>(args[2])) {
                    auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
                    float x = static_cast<float>(std::get<double>(args[0]));
                    float y = static_cast<float>(std::get<double>(args[1]));
                    float z = static_cast<float>(std::get<double>(args[2]));
                    auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
                    if (cmd_buf) {
                        cmd_buf->push([id, x, y, z](ECS::Registry &reg) {
                            if (auto *comp = reg.GetComponent<ECS::Components::TransformComponent>(id)) {
                                comp->transform.SetPosition({x, y, z});
                            }
                        });
                    } else if (reg_ptr) {
                        std::unique_lock lock(g_RegistryMutex);
                        if (auto *comp = reg_ptr->GetComponent<ECS::Components::TransformComponent>(id)) {
                            comp->transform.SetPosition({x, y, z});
                        }
                    }
                }
                return std::monostate{};
            };

            auto set_rot = [id, reg_ptr = &registry](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (args.size() >= 3 && std::holds_alternative<double>(args[0]) && std::holds_alternative<double>(args[1]) && std::holds_alternative<double>(args[2])) {
                    auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
                    float x = static_cast<float>(std::get<double>(args[0]));
                    float y = static_cast<float>(std::get<double>(args[1]));
                    float z = static_cast<float>(std::get<double>(args[2]));
                    auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
                    if (cmd_buf) {
                        cmd_buf->push([id, x, y, z](ECS::Registry &reg) {
                            if (auto *comp = reg.GetComponent<ECS::Components::TransformComponent>(id)) {
                                comp->transform.SetRotation({x, y, z});
                            }
                        });
                    } else if (reg_ptr) {
                        std::unique_lock lock(g_RegistryMutex);
                        if (auto *comp = reg_ptr->GetComponent<ECS::Components::TransformComponent>(id)) {
                            comp->transform.SetRotation({x, y, z});
                        }
                    }
                }
                return std::monostate{};
            };

            auto set_scale = [id, reg_ptr = &registry](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (args.size() >= 3 && std::holds_alternative<double>(args[0]) && std::holds_alternative<double>(args[1]) && std::holds_alternative<double>(args[2])) {
                    auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
                    float x = static_cast<float>(std::get<double>(args[0]));
                    float y = static_cast<float>(std::get<double>(args[1]));
                    float z = static_cast<float>(std::get<double>(args[2]));
                    auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
                    if (cmd_buf) {
                        cmd_buf->push([id, x, y, z](ECS::Registry &reg) {
                            if (auto *comp = reg.GetComponent<ECS::Components::TransformComponent>(id)) {
                                comp->transform.SetScale({x, y, z});
                            }
                        });
                    } else if (reg_ptr) {
                        std::unique_lock lock(g_RegistryMutex);
                        if (auto *comp = reg_ptr->GetComponent<ECS::Components::TransformComponent>(id)) {
                            comp->transform.SetScale({x, y, z});
                        }
                    }
                }
                return std::monostate{};
            };

            auto get_pos = [id, &registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
                std::shared_lock lock(g_RegistryMutex);
                if (const auto *comp = registry.GetComponent<ECS::Components::TransformComponent>(id)) {
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
                std::shared_lock lock(g_RegistryMutex);
                if (const auto *comp = registry.GetComponent<ECS::Components::TransformComponent>(id)) {
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
                std::shared_lock lock(g_RegistryMutex);
                if (const auto *comp = registry.GetComponent<ECS::Components::TransformComponent>(id)) {
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
                std::shared_lock lock(g_RegistryMutex);
                if (auto *move = registry.GetComponent<ECS::Components::MovementComponent>(id)) {
                    return move->isMoving;
                }
                return false;
            };

            obj->fields["SetPosition"] = interpreter->gc.allocate<ObSL::NativeFunction>(3, std::move(set_pos), "SetPosition");
            obj->fields["SetRotation"] = interpreter->gc.allocate<ObSL::NativeFunction>(3, std::move(set_rot), "SetRotation");
            obj->fields["SetScale"] = interpreter->gc.allocate<ObSL::NativeFunction>(3, std::move(set_scale), "SetScale");
            obj->fields["GetPosition"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(get_pos), "GetPosition");
            obj->fields["GetRotation"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(get_rot), "GetRotation");
            obj->fields["GetScale"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(get_scale), "GetScale");
            obj->fields["IsMoving"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(is_moving_body), "IsMoving");

            return obj;
        }

        // POINT LIGHT COMPONENT
        inline ObSL::ObSLObject *CreatePointLightObject(ObSL::Interpreter *interpreter, ECS::Registry &registry, ECS::EntityID id) {
            auto *obj = interpreter->gc.allocate<ObSL::ObSLObject>();
            GCProtectGuard guard(interpreter, obj);

            auto set_color = [id, reg_ptr = &registry](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (args.size() >= 3 && std::holds_alternative<double>(args[0]) && std::holds_alternative<double>(args[1]) && std::holds_alternative<double>(args[2])) {
                    auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
                    float r = static_cast<float>(std::get<double>(args[0]));
                    float g = static_cast<float>(std::get<double>(args[1]));
                    float b = static_cast<float>(std::get<double>(args[2]));
                    auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
                    if (cmd_buf) {
                        cmd_buf->push([id, r, g, b](ECS::Registry &reg) {
                            if (auto *comp = reg.GetComponent<ECS::Components::PointLightComponent>(id))
                                comp->SetColor({r, g, b});
                        });
                    } else if (reg_ptr) {
                        std::unique_lock lock(g_RegistryMutex);
                        if (auto *comp = reg_ptr->GetComponent<ECS::Components::PointLightComponent>(id))
                            comp->SetColor({r, g, b});
                    }
                }
                return std::monostate{};
            };

            auto set_intensity = [id, reg_ptr = &registry](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (!args.empty() && std::holds_alternative<double>(args[0])) {
                    auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
                    float intensity = static_cast<float>(std::get<double>(args[0]));
                    auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
                    if (cmd_buf) {
                        cmd_buf->push([id, intensity](ECS::Registry &reg) {
                            if (auto *comp = reg.GetComponent<ECS::Components::PointLightComponent>(id))
                                comp->SetIntensity(intensity);
                        });
                    } else if (reg_ptr) {
                        std::unique_lock lock(g_RegistryMutex);
                        if (auto *comp = reg_ptr->GetComponent<ECS::Components::PointLightComponent>(id))
                            comp->SetIntensity(intensity);
                    }
                }
                return std::monostate{};
            };

            auto set_radius = [id, reg_ptr = &registry](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (!args.empty() && std::holds_alternative<double>(args[0])) {
                    auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
                    float radius = static_cast<float>(std::get<double>(args[0]));
                    auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
                    if (cmd_buf) {
                        cmd_buf->push([id, radius](ECS::Registry &reg) {
                            if (auto *comp = reg.GetComponent<ECS::Components::PointLightComponent>(id))
                                comp->SetRadius(radius);
                        });
                    } else if (reg_ptr) {
                        std::unique_lock lock(g_RegistryMutex);
                        if (auto *comp = reg_ptr->GetComponent<ECS::Components::PointLightComponent>(id))
                            comp->SetRadius(radius);
                    }
                }
                return std::monostate{};
            };

            obj->fields["SetColor"] = interpreter->gc.allocate<ObSL::NativeFunction>(3, std::move(set_color), "SetColor");
            obj->fields["SetIntensity"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(set_intensity), "SetIntensity");
            obj->fields["SetRadius"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(set_radius), "SetRadius");

            return obj;
        }

        // MOVEMENT COMPONENT
        inline ObSL::ObSLObject *CreateMovementObject(ObSL::Interpreter *interpreter, ECS::Registry &registry, ECS::EntityID id) {
            auto *obj = interpreter->gc.allocate<ObSL::ObSLObject>();
            GCProtectGuard guard(interpreter, obj);

            auto get_is_moving = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                std::shared_lock lock(g_RegistryMutex);
                if (auto *comp = registry.GetComponent<ECS::Components::MovementComponent>(id))
                    return comp->isMoving;
                return false;
            };

            auto set_is_moving = [id, reg_ptr = &registry](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (!args.empty() && std::holds_alternative<bool>(args[0])) {
                    auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
                    bool moving = std::get<bool>(args[0]);
                    auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
                    if (cmd_buf) {
                        cmd_buf->push([id, moving](ECS::Registry &reg) {
                            if (auto *comp = reg.GetComponent<ECS::Components::MovementComponent>(id))
                                comp->isMoving = moving;
                        });
                    } else if (reg_ptr) {
                        std::unique_lock lock(g_RegistryMutex);
                        if (auto *comp = reg_ptr->GetComponent<ECS::Components::MovementComponent>(id))
                            comp->isMoving = moving;
                    }
                }
                return std::monostate{};
            };

            auto set_time_per_step = [id, reg_ptr = &registry](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (!args.empty() && std::holds_alternative<double>(args[0])) {
                    auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
                    float tps = static_cast<float>(std::get<double>(args[0]));
                    auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
                    if (cmd_buf) {
                        cmd_buf->push([id, tps](ECS::Registry &reg) {
                            if (auto *comp = reg.GetComponent<ECS::Components::MovementComponent>(id))
                                comp->timePerStep = tps;
                        });
                    } else if (reg_ptr) {
                        std::unique_lock lock(g_RegistryMutex);
                        if (auto *comp = reg_ptr->GetComponent<ECS::Components::MovementComponent>(id))
                            comp->timePerStep = tps;
                    }
                }
                return std::monostate{};
            };

            obj->fields["GetIsMoving"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(get_is_moving), "GetIsMoving");
            obj->fields["SetIsMoving"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(set_is_moving), "SetIsMoving");
            obj->fields["SetTimePerStep"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(set_time_per_step), "SetTimePerStep");

            return obj;
        }

        // MAP STATE COMPONENT
        inline ObSL::ObSLObject *CreateMapStateObject(ObSL::Interpreter *interpreter, ECS::Registry &registry, ECS::EntityID id) {
            auto *obj = interpreter->gc.allocate<ObSL::ObSLObject>();
            GCProtectGuard guard(interpreter, obj);

            auto get_has_selection = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                if (auto *comp = registry.GetComponent<ECS::Components::MapStateComponent>(id))
                    return comp->hasSelection;
                return false;
            };

            auto get_selected_hex = [id, &registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
                if (const auto *comp = registry.GetComponent<ECS::Components::MapStateComponent>(id)) {
                    auto *arr = interp->gc.allocate<ObSL::ObSLArray>();
                    arr->elements.emplace_back(static_cast<double>(comp->selectedHex.q));
                    arr->elements.emplace_back(static_cast<double>(comp->selectedHex.r));
                    return arr;
                }
                return std::monostate{};
            };

            auto get_path_to_hex = [id, &registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
                if (const auto *comp = registry.GetComponent<ECS::Components::MapStateComponent>(id)) {
                    auto *arr = interp->gc.allocate<ObSL::ObSLArray>();
                    arr->elements.emplace_back(static_cast<double>(comp->pathTo.q));
                    arr->elements.emplace_back(static_cast<double>(comp->pathTo.r));
                    return arr;
                }
                return std::monostate{};
            };

            obj->fields["GetHasSelection"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(get_has_selection), "GetHasSelection");
            obj->fields["GetSelectedHex"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(get_selected_hex), "GetSelectedHex");
            obj->fields["GetPathToHex"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(get_path_to_hex), "GetPathToHex");

            return obj;
        }

        // DIRECTIONAL TEXTURE COMPONENT
        inline ObSL::ObSLObject *CreateDirectionalTextureObject(ObSL::Interpreter *interpreter, ECS::Registry &registry, ECS::EntityID id) {
            auto *obj = interpreter->gc.allocate<ObSL::ObSLObject>();
            GCProtectGuard guard(interpreter, obj);

            auto set_index = [id, reg_ptr = &registry](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (!args.empty() && std::holds_alternative<double>(args[0])) {
                    auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
                    int index = static_cast<int>(std::get<double>(args[0]));
                    auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
                    if (cmd_buf) {
                        cmd_buf->push([id, index](ECS::Registry &reg) {
                            if (auto *comp = reg.GetComponent<ECS::Components::DirectionalTextureComponent>(id))
                                comp->index = index;
                        });
                    } else if (reg_ptr) {
                        std::unique_lock lock(g_RegistryMutex);
                        if (auto *comp = reg_ptr->GetComponent<ECS::Components::DirectionalTextureComponent>(id))
                            comp->index = index;
                    }
                }
                return std::monostate{};
            };

            obj->fields["SetIndex"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(set_index), "SetIndex");
            return obj;
        }

        // TAG COMPONENTS
        inline ObSL::ObSLObject *CreateBillboardTagObject(ObSL::Interpreter *interpreter, ECS::Registry &, ECS::EntityID) { return interpreter->gc.allocate<ObSL::ObSLObject>(); }

        inline ObSL::ObSLObject *CreateDestroyTagObject(ObSL::Interpreter *interpreter, ECS::Registry &, ECS::EntityID) { return interpreter->gc.allocate<ObSL::ObSLObject>(); }

        // PARTICLE EMITTER COMPONENT
        inline ObSL::ObSLObject *CreateParticleEmitterObject(ObSL::Interpreter *interpreter, ECS::Registry &registry, ECS::EntityID id) {
            auto *obj = interpreter->gc.allocate<ObSL::ObSLObject>();
            GCProtectGuard guard(interpreter, obj);

            auto set_emit_rate = [id, reg_ptr = &registry](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (!args.empty() && std::holds_alternative<double>(args[0])) {
                    auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
                    float rate = static_cast<float>(std::get<double>(args[0]));
                    auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
                    if (cmd_buf) {
                        cmd_buf->push([id, rate](ECS::Registry &reg) {
                            if (auto *comp = reg.GetComponent<ECS::Components::ParticleEmitterComponent>(id))
                                comp->emitRate = rate;
                        });
                    } else if (reg_ptr) {
                        std::unique_lock lock(g_RegistryMutex);
                        if (auto *comp = reg_ptr->GetComponent<ECS::Components::ParticleEmitterComponent>(id))
                            comp->emitRate = rate;
                    }
                }
                return std::monostate{};
            };

            auto set_active = [id, reg_ptr = &registry](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (!args.empty()) {
                    bool active = false;
                    if (std::holds_alternative<bool>(args[0])) {
                        active = std::get<bool>(args[0]);
                    } else if (std::holds_alternative<double>(args[0])) {
                        active = std::get<double>(args[0]) != 0.0;
                    }
                    auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
                    auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
                    if (cmd_buf) {
                        cmd_buf->push([id, active](ECS::Registry &reg) {
                            if (auto *comp = reg.GetComponent<ECS::Components::ParticleEmitterComponent>(id))
                                comp->active = active;
                        });
                    } else if (reg_ptr) {
                        std::unique_lock lock(g_RegistryMutex);
                        if (auto *comp = reg_ptr->GetComponent<ECS::Components::ParticleEmitterComponent>(id))
                            comp->active = active;
                    }
                }
                return std::monostate{};
            };

            auto get_active = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                std::shared_lock lock(g_RegistryMutex);
                if (auto *comp = registry.GetComponent<ECS::Components::ParticleEmitterComponent>(id))
                    return comp->active;
                return false;
            };

            auto get_alive_count = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                std::shared_lock lock(g_RegistryMutex);
                if (const auto *comp = registry.GetComponent<ECS::Components::ParticleEmitterComponent>(id))
                    return static_cast<double>(comp->emitterIndex >= 0 ? 0 : 0); // runtime-only, no pool access from script
                return 0.0;
            };

            obj->fields["SetEmitRate"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(set_emit_rate), "SetEmitRate");
            obj->fields["SetActive"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(set_active), "SetActive");
            obj->fields["GetActive"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(get_active), "GetActive");
            obj->fields["GetAliveCount"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(get_alive_count), "GetAliveCount");

            return obj;
        }
    } // namespace EngineLibFactories
} // namespace Scripting
