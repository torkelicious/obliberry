#pragma once
#include "Core/ResourceManager.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

#include <mutex>
#include <shared_mutex>

#include "ECS/Components/ColliderComponent.h"
#include "ECS/Components/SpriteSheetComponent.h"
#include "ECS/Registry.h"
#include <ObSL/Interpreter.h>
#include <ObSL/ScriptWorker.h>
#include <variant>
#include <vector>
#include "ObSL/Parser/ast.h"
#include "Scripting/EngineLib/ScriptCommandBuffer.h"
#include "Scripting/EngineLib/EntityWrapperCache.h"

#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/PointLightComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Components/DirectionalTextureComponent.h"
#include "ECS/Components/ParticleEmitterComponent.h"
#include "ECS/Components/BillboardTagComponent.h"
#include "ECS/Components/DestroyTagComponent.h"


namespace Scripting {
    // Protects all ECS registry access during parallel script execution.
    inline std::shared_mutex g_RegistryMutex;

    // dec so other files can call it
    ObSL::ObSLObject *CreateEntityObject(ObSL::Interpreter *interpreter, ECS::Registry &registry, ECS::EntityID id);
    ObSL::ObSLObject *CreateEntityObjectLocked(ObSL::Interpreter *interpreter, ECS::Registry &registry, ECS::EntityID id);

    namespace EngineLibFactories {
        // GC Guard
        struct GCProtectGuard {
            ObSL::Interpreter *interpreter;

            GCProtectGuard(ObSL::Interpreter *interp, const ObSL::Value &val) : interpreter(interp) { interpreter->gc_protect_stack.emplace_back(val); }

            ~GCProtectGuard() { interpreter->gc_protect_stack.pop_back(); }

            GCProtectGuard(const GCProtectGuard &) = delete;

            GCProtectGuard &operator=(const GCProtectGuard &) = delete;
        };

        // returns cached component wrapper if the entity still has the component
        template <typename Component> ObSL::ObSLObject *LookupCachedComponent(ObSL::Interpreter *interpreter, ECS::Registry &registry, ECS::EntityID id, EntityWrapperCache::Kind kind) {
            if (auto *cache = EntityWrapperCache::Get(interpreter))
                return cache->Find(registry, id, kind, [&] { return registry.IsValid(id) && registry.HasComponent<Component>(id); });
            return nullptr;
        }

        inline void StoreCachedComponent(ObSL::Interpreter *interpreter, const ECS::Registry &registry, const ECS::EntityID id, const EntityWrapperCache::Kind kind, ObSL::ObSLObject *wrapper) {
            if (auto *cache = EntityWrapperCache::Get(interpreter))
                cache->Store(registry, id, kind, wrapper);
        }

        // TRANSFORM COMPONENT
        inline ObSL::ObSLObject *CreateTransformObject(ObSL::Interpreter *interpreter, ECS::Registry &registry, ECS::EntityID id) {
            if (auto *hit = LookupCachedComponent<ECS::Components::TransformComponent>(interpreter, registry, id, EntityWrapperCache::Kind::Transform))
                return hit;

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

            StoreCachedComponent(interpreter, registry, id, EntityWrapperCache::Kind::Transform, obj);

            return obj;
        }

        // POINT LIGHT COMPONENT
        inline ObSL::ObSLObject *CreatePointLightObject(ObSL::Interpreter *interpreter, ECS::Registry &registry, ECS::EntityID id) {
            if (auto *hit = LookupCachedComponent<ECS::Components::PointLightComponent>(interpreter, registry, id, EntityWrapperCache::Kind::PointLight))
                return hit;

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

            StoreCachedComponent(interpreter, registry, id, EntityWrapperCache::Kind::PointLight, obj);

            return obj;
        }

        // MOVEMENT COMPONENT
        inline ObSL::ObSLObject *CreateMovementObject(ObSL::Interpreter *interpreter, ECS::Registry &registry, ECS::EntityID id) {
            if (auto *hit = LookupCachedComponent<ECS::Components::MovementComponent>(interpreter, registry, id, EntityWrapperCache::Kind::Movement))
                return hit;

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

            StoreCachedComponent(interpreter, registry, id, EntityWrapperCache::Kind::Movement, obj);

            return obj;
        }

        // MAP STATE COMPONENT
        inline ObSL::ObSLObject *CreateMapStateObject(ObSL::Interpreter *interpreter, ECS::Registry &registry, ECS::EntityID id) {
            if (auto *hit = LookupCachedComponent<ECS::Components::MapStateComponent>(interpreter, registry, id, EntityWrapperCache::Kind::MapState))
                return hit;

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

            StoreCachedComponent(interpreter, registry, id, EntityWrapperCache::Kind::MapState, obj);

            return obj;
        }

        // DIRECTIONAL TEXTURE COMPONENT
        inline ObSL::ObSLObject *CreateDirectionalTextureObject(ObSL::Interpreter *interpreter, ECS::Registry &registry, ECS::EntityID id) {
            if (auto *hit = LookupCachedComponent<ECS::Components::DirectionalTextureComponent>(interpreter, registry, id, EntityWrapperCache::Kind::DirectionalTexture))
                return hit;

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

            StoreCachedComponent(interpreter, registry, id, EntityWrapperCache::Kind::DirectionalTexture, obj);

            return obj;
        }

        // TAG COMPONENTS
        inline ObSL::ObSLObject *CreateBillboardTagObject(ObSL::Interpreter *interpreter, ECS::Registry &registry, const ECS::EntityID id) {
            if (auto *hit = LookupCachedComponent<ECS::Components::BillboardTagComponent>(interpreter, registry, id, EntityWrapperCache::Kind::BillboardTag))
                return hit;
            auto *obj = interpreter->gc.allocate<ObSL::ObSLObject>();
            StoreCachedComponent(interpreter, registry, id, EntityWrapperCache::Kind::BillboardTag, obj);
            return obj;
        }

        inline ObSL::ObSLObject *CreateDestroyTagObject(ObSL::Interpreter *interpreter, ECS::Registry &registry, const ECS::EntityID id) {
            if (auto *hit = LookupCachedComponent<ECS::Components::DestroyTagComponent>(interpreter, registry, id, EntityWrapperCache::Kind::DestroyTag))
                return hit;
            auto *obj = interpreter->gc.allocate<ObSL::ObSLObject>();
            StoreCachedComponent(interpreter, registry, id, EntityWrapperCache::Kind::DestroyTag, obj);
            return obj;
        }

        // PARTICLE EMITTER COMPONENT
        inline ObSL::ObSLObject *CreateParticleEmitterObject(ObSL::Interpreter *interpreter, ECS::Registry &registry, ECS::EntityID id) {
            if (auto *hit = LookupCachedComponent<ECS::Components::ParticleEmitterComponent>(interpreter, registry, id, EntityWrapperCache::Kind::ParticleEmitter))
                return hit;

            auto *obj = interpreter->gc.allocate<ObSL::ObSLObject>();
            GCProtectGuard guard(interpreter, obj);

            auto set_emit_rate = [id, reg_ptr = &registry](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (!args.empty() && std::holds_alternative<double>(args[0])) {
                    auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
                    float rate = static_cast<float>(std::get<double>(args[0]));
                    auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
                    if (cmd_buf) {
                        cmd_buf->push([id, rate](ECS::Registry &reg) {
                            if (auto *comp = reg.GetComponent<ECS::Components::ParticleEmitterComponent>(id)) {
                                comp->emitRate = rate;
                                comp->isDirty = true;
                            }
                        });
                    } else if (reg_ptr) {
                        std::unique_lock lock(g_RegistryMutex);
                        if (auto *comp = reg_ptr->GetComponent<ECS::Components::ParticleEmitterComponent>(id)) {
                            comp->emitRate = rate;
                            comp->isDirty = true;
                        }
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
                            if (auto *comp = reg.GetComponent<ECS::Components::ParticleEmitterComponent>(id)) {
                                comp->active = active;
                                comp->isDirty = true;
                            }
                        });
                    } else if (reg_ptr) {
                        std::unique_lock lock(g_RegistryMutex);
                        if (auto *comp = reg_ptr->GetComponent<ECS::Components::ParticleEmitterComponent>(id)) {
                            comp->active = active;
                            comp->isDirty = true;
                        }
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
                    return static_cast<double>(comp->emitterIndex >= 0 ? comp->emitterIndex : 0); // runtime-only, no pool access from script
                return 0.0;
            };

            obj->fields["SetEmitRate"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(set_emit_rate), "SetEmitRate");
            obj->fields["SetActive"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(set_active), "SetActive");
            obj->fields["GetActive"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(get_active), "GetActive");
            obj->fields["GetAliveCount"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(get_alive_count), "GetAliveCount");

            StoreCachedComponent(interpreter, registry, id, EntityWrapperCache::Kind::ParticleEmitter, obj);

            return obj;
        }

        inline ObSL::ObSLObject *CreateColliderObject(ObSL::Interpreter *interpreter, ECS::Registry &registry, ECS::EntityID id) {
            using Component = ECS::Components::ColliderComponent;
            if (auto *hit = LookupCachedComponent<Component>(interpreter, registry, id, EntityWrapperCache::Kind::Collider)) {
                return hit;
            }

            auto *obj = interpreter->gc.allocate<ObSL::ObSLObject>();
            GCProtectGuard guard(interpreter, obj);

            auto get_trigger = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                std::shared_lock lock(g_RegistryMutex);
                if (registry.IsValid(id)) {
                    if (auto *comp = registry.GetComponent<Component>(id)) {
                        return comp->isTrigger;
                    }
                }
                return std::monostate{};
            };

            auto set_trigger = [id, &registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (args.empty() || !std::holds_alternative<bool>(args[0])) {
                    return std::monostate{};
                }

                const bool value = std::get<bool>(args[0]);

                auto apply = [id, value](ECS::Registry &reg) {
                    if (!reg.IsValid(id)) {
                        return;
                    }
                    if (auto *comp = reg.GetComponent<Component>(id)) {
                        comp->isTrigger = value;
                    }
                };

                auto *worker = static_cast<ObSL::ScriptWorker *>(interp->user_data);
                auto *commands = worker ? worker->frame_context<ScriptCommandBuffer>() : nullptr;

                if (commands) {
                    commands->push(std::move(apply));
                } else {
                    std::unique_lock lock(g_RegistryMutex);
                    apply(registry);
                }
                return std::monostate{};
            };

            obj->fields["GetIsTrigger"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(get_trigger), "GetIsTrigger");
            obj->fields["SetIsTrigger"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(set_trigger), "SetIsTrigger");

            StoreCachedComponent(interpreter, registry, id, EntityWrapperCache::Kind::Collider, obj);

            return obj;
        }


        // helpers
        namespace ComponentBinding {
            inline double Number(const std::vector<ObSL::Value> &args, size_t index) {
                if (index >= args.size() || !std::holds_alternative<double>(args[index]))
                    throw std::runtime_error("Expected a numeric argument");
                double value = std::get<double>(args[index]);
                if (!std::isfinite(value))
                    throw std::runtime_error("Expected a finite number");
                return value;
            }
            inline int Integer(const std::vector<ObSL::Value> &args, size_t index, int lo, int hi) {
                double value = Number(args, index);
                if (value < lo || value > hi || std::trunc(value) != value)
                    throw std::runtime_error("Integer argument is out of range");
                return static_cast<int>(value);
            }
            inline float Float(const std::vector<ObSL::Value> &args, size_t index, bool positive = false) {
                double value = Number(args, index);
                if (std::abs(value) > std::numeric_limits<float>::max())
                    throw std::runtime_error("Float argument is out of range");
                float result = static_cast<float>(value);
                if (positive && result <= 0.0f)
                    throw std::runtime_error("Expected a positive float");
                return result;
            }
            inline bool Boolean(const std::vector<ObSL::Value> &args, size_t index) {
                if (index >= args.size() || !std::holds_alternative<bool>(args[index]))
                    throw std::runtime_error("Expected a boolean argument");
                return std::get<bool>(args[index]);
            }
            inline std::string String(const std::vector<ObSL::Value> &args, size_t index) {
                if (index >= args.size() || !std::holds_alternative<std::string>(args[index]))
                    throw std::runtime_error("Expected a string argument");
                return std::get<std::string>(args[index]);
            }
            template <typename F> void Method(ObSL::Interpreter *interp, ObSL::ObSLObject *obj, const char *name, int arity, F fn) {
                obj->fields[name] = interp->gc.allocate<ObSL::NativeFunction>(arity, std::move(fn), name);
            }
            template <typename C, typename F> ObSL::Value Read(ObSL::Interpreter *interp, ECS::Registry &reg, ECS::EntityID id, F fn) {
                std::shared_lock lock(g_RegistryMutex);
                if (reg.IsValid(id)) {
                    if (auto *comp = reg.GetComponent<C>(id))
                        return fn(interp, *comp);
                }
                return std::monostate{};
            }
            template <typename C, typename F> ObSL::Value Write(ObSL::Interpreter *interp, ECS::Registry &reg, ECS::EntityID id, F fn) {
                auto apply = [id, fn = std::move(fn)](ECS::Registry &target) mutable {
                    if (!target.IsValid(id))
                        return;
                    if (auto *comp = target.GetComponent<C>(id))
                        fn(*comp);
                };
                auto *worker = static_cast<ObSL::ScriptWorker *>(interp->user_data);
                auto *commands = worker ? worker->frame_context<ScriptCommandBuffer>() : nullptr;
                if (commands)
                    commands->push(std::move(apply));
                else {
                    std::unique_lock lock(g_RegistryMutex);
                    apply(reg);
                }
                return std::monostate{};
            }
            inline ObSL::Value Vector(ObSL::Interpreter *interp, const glm::vec3 &v) {
                auto *arr = interp->gc.allocate<ObSL::ObSLArray>();
                GCProtectGuard guard(interp, arr);
                arr->elements.emplace_back(static_cast<double>(v.x));
                arr->elements.emplace_back(static_cast<double>(v.y));
                arr->elements.emplace_back(static_cast<double>(v.z));
                return arr;
            }
            inline int TotalFrames(const ECS::Components::SpriteSheetComponent &c) {
                if (!c.sheet || c.sheet->columns <= 0 || c.sheet->rows <= 0)
                    return 0;
                const int64_t total = int64_t(c.sheet->columns) * c.sheet->rows;
                return total <= std::numeric_limits<int>::max() ? static_cast<int>(total) : 0;
            }
            inline bool ValidClip(const ECS::Components::SpriteSheetComponent &c) {
                const int total = TotalFrames(c);
                return c.startFrame >= 0 && c.startFrame < total && c.frameCount > 0 && c.frameCount <= total - c.startFrame;
            }
            inline std::shared_ptr<Rendering::SpriteSheet> CopySheet(const ECS::Components::SpriteSheetComponent &c) {
                return c.sheet ? std::make_shared<Rendering::SpriteSheet>(*c.sheet) : std::make_shared<Rendering::SpriteSheet>();
            }
        } // namespace ComponentBinding

        inline ObSL::ObSLObject *CreateSpriteSheetObject(ObSL::Interpreter *interpreter, ECS::Registry &registry, ECS::EntityID id) {
            using C = ECS::Components::SpriteSheetComponent;
            using namespace ComponentBinding;
            if (!registry.IsValid(id) || !registry.HasComponent<C>(id))
                return nullptr;
            if (auto *hit = LookupCachedComponent<C>(interpreter, registry, id, EntityWrapperCache::Kind::SpriteSheet))
                return hit;
            auto *obj = interpreter->gc.allocate<ObSL::ObSLObject>();
            GCProtectGuard guard(interpreter, obj);
            Method(interpreter, obj, "GetFrame", 0, [id, &registry](ObSL::Interpreter *i, const std::vector<ObSL::Value> &) -> ObSL::Value {
                return Read<C>(i, registry, id, [](ObSL::Interpreter *, const C &c) -> ObSL::Value { return static_cast<double>(c.currentFrame); });
            });
            Method(interpreter, obj, "GetStartFrame", 0, [id, &registry](ObSL::Interpreter *i, const std::vector<ObSL::Value> &) -> ObSL::Value {
                return Read<C>(i, registry, id, [](ObSL::Interpreter *, const C &c) -> ObSL::Value { return static_cast<double>(c.startFrame); });
            });
            Method(interpreter, obj, "GetFrameCount", 0, [id, &registry](ObSL::Interpreter *i, const std::vector<ObSL::Value> &) -> ObSL::Value {
                return Read<C>(i, registry, id, [](ObSL::Interpreter *, const C &c) -> ObSL::Value { return static_cast<double>(c.frameCount); });
            });
            Method(interpreter, obj, "GetFPS", 0, [id, &registry](ObSL::Interpreter *i, const std::vector<ObSL::Value> &) -> ObSL::Value {
                return Read<C>(i, registry, id, [](ObSL::Interpreter *, const C &c) -> ObSL::Value { return static_cast<double>(c.framesPerSecond); });
            });
            Method(interpreter, obj, "GetLoop", 0,
                   [id, &registry](ObSL::Interpreter *i, const std::vector<ObSL::Value> &) -> ObSL::Value { return Read<C>(i, registry, id, [](ObSL::Interpreter *, const C &c) -> ObSL::Value { return c.loop; }); });
            Method(interpreter, obj, "IsPlaying", 0,
                   [id, &registry](ObSL::Interpreter *i, const std::vector<ObSL::Value> &) -> ObSL::Value { return Read<C>(i, registry, id, [](ObSL::Interpreter *, const C &c) -> ObSL::Value { return c.playing; }); });
            Method(interpreter, obj, "GetColumns", 0, [id, &registry](ObSL::Interpreter *i, const std::vector<ObSL::Value> &) -> ObSL::Value {
                return Read<C>(i, registry, id, [](ObSL::Interpreter *, const C &c) -> ObSL::Value { return static_cast<double>(c.sheet ? c.sheet->columns : 0); });
            });
            Method(interpreter, obj, "GetRows", 0, [id, &registry](ObSL::Interpreter *i, const std::vector<ObSL::Value> &) -> ObSL::Value {
                return Read<C>(i, registry, id, [](ObSL::Interpreter *, const C &c) -> ObSL::Value { return static_cast<double>(c.sheet ? c.sheet->rows : 0); });
            });
            Method(interpreter, obj, "GetTexture", 0, [id, &registry](ObSL::Interpreter *i, const std::vector<ObSL::Value> &) -> ObSL::Value {
                return Read<C>(i, registry, id, [](ObSL::Interpreter *, const C &c) -> ObSL::Value { return Core::ResourceManager::GetInstance().GetKey<Rendering::Texture>(c.sheet ? c.sheet->texture : nullptr); });
            });
            Method(interpreter, obj, "SetTexture", 1, [id, &registry](ObSL::Interpreter *i, const std::vector<ObSL::Value> &a) -> ObSL::Value {
                const auto key = String(a, 0);
                auto texture = key.empty() ? nullptr : Core::ResourceManager::GetInstance().Get<Rendering::Texture>(key);
                if (!key.empty() && !texture)
                    throw std::runtime_error("Spritesheet texture resource not found: " + key);
                return Write<C>(i, registry, id, [texture = std::move(texture)](C &c) {
                    auto sheet = CopySheet(c);
                    sheet->texture = texture;
                    c.sheet = std::move(sheet);
                });
            });
            Method(interpreter, obj, "SetGrid", 2, [id, &registry](ObSL::Interpreter *i, const std::vector<ObSL::Value> &a) -> ObSL::Value {
                const int columns = Integer(a, 0, 1, std::numeric_limits<int>::max());
                const int rows = Integer(a, 1, 1, std::numeric_limits<int>::max());
                if (int64_t(columns) * rows > std::numeric_limits<int>::max())
                    throw std::runtime_error("Spritesheet grid is too large");
                return Write<C>(i, registry, id, [columns, rows](C &c) {
                    auto sheet = CopySheet(c);
                    sheet->columns = columns;
                    sheet->rows = rows;
                    c.sheet = std::move(sheet);
                    const int total = TotalFrames(c);
                    c.startFrame = std::clamp(c.startFrame, 0, total - 1);
                    c.frameCount = std::clamp(c.frameCount, 1, total - c.startFrame);
                    c.currentFrame = std::clamp(c.currentFrame, 0, c.frameCount - 1);
                    c.elapsed = 0.0f;
                });
            });
            Method(interpreter, obj, "SetAnimation", 4, [id, &registry](ObSL::Interpreter *i, const std::vector<ObSL::Value> &a) -> ObSL::Value {
                const int start = Integer(a, 0, 0, std::numeric_limits<int>::max());
                const int count = Integer(a, 1, 1, std::numeric_limits<int>::max());
                const float fps = Float(a, 2, true);
                if (fps > 240.0f)
                    throw std::runtime_error("FPS must be at most 240");
                const bool loop = Boolean(a, 3);
                return Write<C>(i, registry, id, [start, count, fps, loop](C &c) {
                    const int total = TotalFrames(c);
                    // Validate against grid state at execution, including earlier queued SetGrid calls.
                    if (start >= total || count > total - start)
                        return;
                    c.startFrame = start;
                    c.frameCount = count;
                    c.framesPerSecond = fps;
                    c.loop = loop;
                    c.currentFrame = 0;
                    c.elapsed = 0.0f;
                });
            });
            Method(interpreter, obj, "SetFrame", 1, [id, &registry](ObSL::Interpreter *i, const std::vector<ObSL::Value> &a) -> ObSL::Value {
                const int frame = Integer(a, 0, 0, std::numeric_limits<int>::max());
                return Write<C>(i, registry, id, [frame](C &c) {
                    if (!ValidClip(c) || frame >= c.frameCount)
                        return;
                    c.currentFrame = frame;
                    c.elapsed = 0.0f;
                    c.playing = false;
                });
            });
            Method(interpreter, obj, "Play", 0, [id, &registry](ObSL::Interpreter *i, const std::vector<ObSL::Value> &) -> ObSL::Value {
                return Write<C>(i, registry, id, [](C &c) {
                    if (ValidClip(c))
                        c.playing = true;
                });
            });
            Method(interpreter, obj, "Pause", 0, [id, &registry](ObSL::Interpreter *i, const std::vector<ObSL::Value> &) -> ObSL::Value { return Write<C>(i, registry, id, [](C &c) { c.playing = false; }); });
            Method(interpreter, obj, "Restart", 0, [id, &registry](ObSL::Interpreter *i, const std::vector<ObSL::Value> &) -> ObSL::Value {
                return Write<C>(i, registry, id, [](C &c) {
                    if (ValidClip(c)) {
                        c.currentFrame = 0;
                        c.elapsed = 0.0f;
                        c.playing = true;
                    }
                });
            });
            Method(interpreter, obj, "Stop", 0, [id, &registry](ObSL::Interpreter *i, const std::vector<ObSL::Value> &) -> ObSL::Value {
                return Write<C>(i, registry, id, [](C &c) {
                    c.currentFrame = 0;
                    c.elapsed = 0.0f;
                    c.playing = false;
                });
            });
            StoreCachedComponent(interpreter, registry, id, EntityWrapperCache::Kind::SpriteSheet, obj);
            return obj;
        }
    } // namespace EngineLibFactories
} // namespace Scripting
