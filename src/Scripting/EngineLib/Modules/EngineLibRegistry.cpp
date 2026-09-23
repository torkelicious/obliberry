#include "Scripting/EngineLib/EngineLib.h"
#include "Scripting/EngineLib/EngineLibFactories.h"
#include "Scripting/EngineLib/EntityWrapperCache.h"
#include "Scripting/EngineLib/ScriptCommandBuffer.h"

#include "ECS/Entity.h"
#include "ECS/Registry.h"
#include "ECS/Components/BillboardTagComponent.h"
#include "ECS/Components/ColliderComponent.h"
#include "ECS/Components/CustomDataComponent.h"
#include "ECS/Components/DestroyTagComponent.h"
#include "ECS/Components/DirectionalTextureComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/ParticleEmitterComponent.h"
#include "ECS/Components/PersistentTagComponent.h"
#include "ECS/Components/PointLightComponent.h"
#include "ECS/Components/RelationshipComponent.h"
#include "ECS/Components/SpriteAnimatorComponent.h"
#include "ECS/Components/SpriteSheetComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "IO/Loaders/PrefabManager.h"
#include <ObSL/ScriptWorker.h>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace Scripting {

    // if the caller already hold Registry Mutex!!
    static ObSL::ObSLArray *ChildrenAsArray(ObSL::Interpreter *interp, ECS::Registry &reg, const ECS::EntityID id) {
        auto *arr = interp->gc.allocate<ObSL::ObSLArray>();
        EngineLibFactories::GCProtectGuard guard(interp, arr);
        if (reg.IsValid(id)) {
            const ECS::Entity entity(id, &reg);

            for (const auto child : entity.GetChildren()) {
                if (reg.IsValid(child)) {
                    arr->elements.emplace_back(CreateEntityObjectLocked(interp, reg, child));
                }
            }
        }

        return arr;
    }

    // cache hit reuses a previously built wrapper
    // when the entity is still live and belongs to the same registry.
    // names will be updated if such functions are called
    ObSL::ObSLObject *CreateEntityObjectLocked(ObSL::Interpreter *interpreter, ECS::Registry &registry, ECS::EntityID id) {
        if (!registry.IsValid(id)) {
            return nullptr;
        }

        if (auto *cache = EntityWrapperCache::Get(interpreter)) {
            if (auto *hit = cache->Find(registry, id, EntityWrapperCache::Kind::Entity, [&] { return registry.IsValid(id); })) {
                hit->fields["id"] = static_cast<double>(id);
                hit->fields["name"] = registry.GetEntityName(id);
                return hit;
            }
        }

        auto *obj = interpreter->gc.allocate<ObSL::ObSLObject>();
        EngineLibFactories::GCProtectGuard guard(interpreter, obj);

        obj->fields["id"] = static_cast<double>(id);
        obj->fields["name"] = registry.GetEntityName(id);

        auto set_name_body = [id, reg_ptr = &registry](const ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.empty() || !std::holds_alternative<std::string>(args[0])) {
                return std::monostate{};
            }

            auto name = std::get<std::string>(args[0]);

            auto apply = [id, name = std::move(name)](ECS::Registry &reg) {
                if (!reg.IsValid(id)) {
                    return;
                }

                reg.SetEntityName(id, name);
            };

            auto *worker = static_cast<ObSL::ScriptWorker *>(interp->user_data);
            auto *commands = worker ? worker->frame_context<ScriptCommandBuffer>() : nullptr;

            if (commands) {
                commands->push(std::move(apply));
            } else {
                std::unique_lock lock(g_RegistryMutex);
                apply(*reg_ptr);
            }

            return std::monostate{};
        };

        auto get_name_body = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
            std::shared_lock lock(g_RegistryMutex);

            if (!registry.IsValid(id)) {
                return std::monostate{};
            }

            return registry.GetEntityName(id);
        };

        auto get_comp_body = [id, &registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.empty() || !std::holds_alternative<std::string>(args[0])) {
                return std::monostate{};
            }

            std::shared_lock lock(g_RegistryMutex);

            if (!registry.IsValid(id)) {
                return std::monostate{};
            }

            const auto &name = std::get<std::string>(args[0]);

            if (name == "Transform") {
                return EngineLibFactories::CreateTransformObject(interp, registry, id);
            }
            if (name == "PointLight") {
                return EngineLibFactories::CreatePointLightObject(interp, registry, id);
            }
            if (name == "Movement") {
                return EngineLibFactories::CreateMovementObject(interp, registry, id);
            }
            if (name == "MapState") {
                return EngineLibFactories::CreateMapStateObject(interp, registry, id);
            }
            if (name == "DirectionalTexture") {
                return EngineLibFactories::CreateDirectionalTextureObject(interp, registry, id);
            }
            if (name == "BillboardTag") {
                return EngineLibFactories::CreateBillboardTagObject(interp, registry, id);
            }
            if (name == "DestroyTag") {
                return EngineLibFactories::CreateDestroyTagObject(interp, registry, id);
            }
            if (name == "ParticleEmitter") {
                return EngineLibFactories::CreateParticleEmitterObject(interp, registry, id);
            }
            if (name == "Collider") {
                if (!registry.HasComponent<ECS::Components::ColliderComponent>(id)) {
                    return std::monostate{};
                }

                return EngineLibFactories::CreateColliderObject(interp, registry, id);
            }
            if (name == "SpriteSheet") {
                if (!registry.HasComponent<ECS::Components::SpriteSheetComponent>(id)) {
                    return std::monostate{};
                }

                return EngineLibFactories::CreateSpriteSheetObject(interp, registry, id);
            }
            if (name == "SpriteAnimator") {
                if (!registry.HasComponent<ECS::Components::SpriteAnimatorComponent>(id)) {
                    return std::monostate{};
                }

                return EngineLibFactories::CreateSpriteAnimatorObject(interp, registry, id);
            }

            return std::monostate{};
        };

        auto add_comp_body = [id, reg_ptr = &registry](const ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.empty() || !std::holds_alternative<std::string>(args[0])) {
                return std::monostate{};
            }

            const auto name = std::get<std::string>(args[0]);

            auto apply = [id, name](ECS::Registry &reg) {
                using namespace ECS::Components;

                if (!reg.IsValid(id)) {
                    return;
                }

                if (name == "Transform") {
                    if (!reg.HasComponent<TransformComponent>(id)) {
                        reg.AddComponent<TransformComponent>(id, TransformComponent{});
                    }
                } else if (name == "PointLight") {
                    if (!reg.HasComponent<PointLightComponent>(id)) {
                        reg.AddComponent<PointLightComponent>(id, PointLightComponent{});
                    }
                } else if (name == "Movement") {
                    if (!reg.HasComponent<MovementComponent>(id)) {
                        reg.AddComponent<MovementComponent>(id, MovementComponent{});
                    }
                } else if (name == "MapState") {
                    if (!reg.HasComponent<MapStateComponent>(id)) {
                        reg.AddComponent<MapStateComponent>(id, MapStateComponent{});
                    }
                } else if (name == "DirectionalTexture") {
                    if (!reg.HasComponent<DirectionalTextureComponent>(id)) {
                        reg.AddComponent<DirectionalTextureComponent>(id, DirectionalTextureComponent{});
                    }
                } else if (name == "BillboardTag") {
                    if (!reg.HasComponent<BillboardTagComponent>(id)) {
                        reg.AddComponent<BillboardTagComponent>(id, BillboardTagComponent{});
                    }
                } else if (name == "DestroyTag") {
                    if (!reg.HasComponent<DestroyTagComponent>(id)) {
                        reg.AddComponent<DestroyTagComponent>(id, DestroyTagComponent{});
                    }
                } else if (name == "ParticleEmitter") {
                    if (!reg.HasComponent<ParticleEmitterComponent>(id)) {
                        reg.AddComponent<ParticleEmitterComponent>(id, ParticleEmitterComponent{});
                    }
                } else if (name == "Collider") {
                    if (!reg.HasComponent<ColliderComponent>(id)) {
                        reg.AddComponent<ColliderComponent>(id, ColliderComponent{});
                    }
                } else if (name == "SpriteSheet") {
                    if (!reg.HasComponent<SpriteSheetComponent>(id)) {
                        reg.AddComponent<SpriteSheetComponent>(id, SpriteSheetComponent{});
                    }
                } else if (name == "SpriteAnimator") {
                    if (!reg.HasComponent<SpriteSheetComponent>(id)) {
                        reg.AddComponent<SpriteSheetComponent>(id, SpriteSheetComponent{});
                    }

                    if (!reg.HasComponent<SpriteAnimatorComponent>(id)) {
                        reg.AddComponent<SpriteAnimatorComponent>(id, SpriteAnimatorComponent{});
                    }
                }
            };

            auto *worker = static_cast<ObSL::ScriptWorker *>(interp->user_data);
            auto *commands = worker ? worker->frame_context<ScriptCommandBuffer>() : nullptr;

            if (commands) {
                commands->push(std::move(apply));
            } else {
                std::unique_lock lock(g_RegistryMutex);
                apply(*reg_ptr);
            }

            return std::monostate{};
        };

        auto has_comp_body = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            using namespace ECS::Components;

            if (args.empty() || !std::holds_alternative<std::string>(args[0])) {
                return false;
            }

            std::shared_lock lock(g_RegistryMutex);

            if (!registry.IsValid(id)) {
                return false;
            }

            const auto &name = std::get<std::string>(args[0]);

            if (name == "Transform") {
                return registry.HasComponent<TransformComponent>(id);
            }
            if (name == "PointLight") {
                return registry.HasComponent<PointLightComponent>(id);
            }
            if (name == "Movement") {
                return registry.HasComponent<MovementComponent>(id);
            }
            if (name == "MapState") {
                return registry.HasComponent<MapStateComponent>(id);
            }
            if (name == "DirectionalTexture") {
                return registry.HasComponent<DirectionalTextureComponent>(id);
            }
            if (name == "BillboardTag") {
                return registry.HasComponent<BillboardTagComponent>(id);
            }
            if (name == "DestroyTag") {
                return registry.HasComponent<DestroyTagComponent>(id);
            }
            if (name == "ParticleEmitter") {
                return registry.HasComponent<ParticleEmitterComponent>(id);
            }
            if (name == "Collider") {
                return registry.HasComponent<ColliderComponent>(id);
            }
            if (name == "SpriteSheet") {
                return registry.HasComponent<SpriteSheetComponent>(id);
            }
            if (name == "SpriteAnimator") {
                return registry.HasComponent<SpriteAnimatorComponent>(id);
            }

            return false;
        };

        auto remove_comp_body = [id, reg_ptr = &registry](const ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.empty() || !std::holds_alternative<std::string>(args[0])) {
                return std::monostate{};
            }

            const auto name = std::get<std::string>(args[0]);

            auto apply = [id, name](ECS::Registry &reg) {
                using namespace ECS::Components;

                if (!reg.IsValid(id)) {
                    return;
                }

                if (name == "Transform") {
                    reg.RemoveComponent<TransformComponent>(id);
                } else if (name == "PointLight") {
                    reg.RemoveComponent<PointLightComponent>(id);
                } else if (name == "Movement") {
                    reg.RemoveComponent<MovementComponent>(id);
                } else if (name == "MapState") {
                    reg.RemoveComponent<MapStateComponent>(id);
                } else if (name == "DirectionalTexture") {
                    reg.RemoveComponent<DirectionalTextureComponent>(id);
                } else if (name == "BillboardTag") {
                    reg.RemoveComponent<BillboardTagComponent>(id);
                } else if (name == "DestroyTag") {
                    reg.RemoveComponent<DestroyTagComponent>(id);
                } else if (name == "ParticleEmitter") {
                    reg.RemoveComponent<ParticleEmitterComponent>(id);
                } else if (name == "Collider") {
                    reg.RemoveComponent<ColliderComponent>(id);
                } else if (name == "SpriteSheet") {
                    reg.RemoveComponent<SpriteSheetComponent>(id);
                } else if (name == "SpriteAnimator") {
                    reg.RemoveComponent<SpriteAnimatorComponent>(id);
                }
            };

            auto *worker = static_cast<ObSL::ScriptWorker *>(interp->user_data);
            auto *commands = worker ? worker->frame_context<ScriptCommandBuffer>() : nullptr;

            if (commands) {
                commands->push(std::move(apply));
            } else {
                std::unique_lock lock(g_RegistryMutex);
                apply(*reg_ptr);
            }

            return std::monostate{};
        };

        auto get_components_body = [id, &registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
            using namespace ECS::Components;

            std::shared_lock lock(g_RegistryMutex);

            if (!registry.IsValid(id)) {
                return std::monostate{};
            }

            auto *arr = interp->gc.allocate<ObSL::ObSLArray>();

            if (registry.HasComponent<TransformComponent>(id)) {
                arr->elements.emplace_back(std::string("Transform"));
            }
            if (registry.HasComponent<PointLightComponent>(id)) {
                arr->elements.emplace_back(std::string("PointLight"));
            }
            if (registry.HasComponent<MovementComponent>(id)) {
                arr->elements.emplace_back(std::string("Movement"));
            }
            if (registry.HasComponent<MapStateComponent>(id)) {
                arr->elements.emplace_back(std::string("MapState"));
            }
            if (registry.HasComponent<DirectionalTextureComponent>(id)) {
                arr->elements.emplace_back(std::string("DirectionalTexture"));
            }
            if (registry.HasComponent<BillboardTagComponent>(id)) {
                arr->elements.emplace_back(std::string("BillboardTag"));
            }
            if (registry.HasComponent<DestroyTagComponent>(id)) {
                arr->elements.emplace_back(std::string("DestroyTag"));
            }
            if (registry.HasComponent<ParticleEmitterComponent>(id)) {
                arr->elements.emplace_back(std::string("ParticleEmitter"));
            }
            if (registry.HasComponent<ColliderComponent>(id)) {
                arr->elements.emplace_back(std::string("Collider"));
            }
            if (registry.HasComponent<SpriteSheetComponent>(id)) {
                arr->elements.emplace_back(std::string("SpriteSheet"));
            }
            if (registry.HasComponent<SpriteAnimatorComponent>(id)) {
                arr->elements.emplace_back(std::string("SpriteAnimator"));
            }

            return arr;
        };

        auto destroy_body = [id, reg_ptr = &registry](const ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
            auto *worker = static_cast<ObSL::ScriptWorker *>(interp->user_data);
            auto *commands = worker ? worker->frame_context<ScriptCommandBuffer>() : nullptr;

            if (commands) {
                commands->push([id](ECS::Registry &reg) { reg.DestroyEntity(id); });
            } else {
                std::unique_lock lock(g_RegistryMutex);
                reg_ptr->DestroyEntity(id);
            }

            return std::monostate{};
        };

        auto add_custom_comp = [id, reg_ptr = &registry](const ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.size() != 2 || !std::holds_alternative<std::string>(args[0])) {
                return false;
            }

            auto compName = std::get<std::string>(args[0]);
            ObSL::Value value = args[1];

            auto apply = [id, compName = std::move(compName), value = std::move(value)](ECS::Registry &reg) {
                if (!reg.IsValid(id)) {
                    return;
                }

                if (!reg.HasComponent<ECS::Components::CustomDataComponent>(id)) {
                    reg.AddComponent<ECS::Components::CustomDataComponent>(id, ECS::Components::CustomDataComponent{});
                }

                if (auto *comp = reg.GetComponent<ECS::Components::CustomDataComponent>(id)) {
                    comp->script_components[compName] = value;
                }
            };

            auto *worker = static_cast<ObSL::ScriptWorker *>(interp->user_data);
            auto *commands = worker ? worker->frame_context<ScriptCommandBuffer>() : nullptr;

            if (commands) {
                commands->push(std::move(apply));
            } else {
                std::unique_lock lock(g_RegistryMutex);
                apply(*reg_ptr);
            }

            return true;
        };

        auto get_custom_comp = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.size() != 1 || !std::holds_alternative<std::string>(args[0])) {
                return std::monostate{};
            }

            std::shared_lock lock(g_RegistryMutex);

            if (!registry.IsValid(id)) {
                return std::monostate{};
            }

            if (auto *comp = registry.GetComponent<ECS::Components::CustomDataComponent>(id)) {
                const auto &name = std::get<std::string>(args[0]);
                const auto it = comp->script_components.find(name);

                if (it != comp->script_components.end()) {
                    return it->second;
                }
            }

            return std::monostate{};
        };

        auto set_persistent_body = [id, reg_ptr = &registry](const ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.empty() || !std::holds_alternative<bool>(args[0])) {
                return std::monostate{};
            }

            const bool shouldPersist = std::get<bool>(args[0]);

            auto apply = [id, shouldPersist](ECS::Registry &reg) {
                if (!reg.IsValid(id)) {
                    return;
                }

                if (shouldPersist) {
                    reg.AddComponent<ECS::Components::PersistentTagComponent>(id);
                } else {
                    reg.RemoveComponent<ECS::Components::PersistentTagComponent>(id);
                }
            };

            auto *worker = static_cast<ObSL::ScriptWorker *>(interp->user_data);
            auto *commands = worker ? worker->frame_context<ScriptCommandBuffer>() : nullptr;

            if (commands) {
                commands->push(std::move(apply));
            } else {
                std::unique_lock lock(g_RegistryMutex);
                apply(*reg_ptr);
            }

            return std::monostate{};
        };

        auto is_persistent_body = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
            std::shared_lock lock(g_RegistryMutex);

            if (!registry.IsValid(id)) {
                return false;
            }

            return registry.HasComponent<ECS::Components::PersistentTagComponent>(id);
        };

        auto get_children_body = [id, &registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
            std::shared_lock lock(g_RegistryMutex);
            return ChildrenAsArray(interp, registry, id);
        };

        auto get_parent_body = [id, &registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
            std::shared_lock lock(g_RegistryMutex);

            if (!registry.IsValid(id)) {
                return std::monostate{};
            }

            const auto *rel = registry.GetComponent<ECS::Components::RelationshipComponent>(id);

            if (!rel || rel->parent == ECS::INVALID_ENTITY_ID || !registry.IsValid(rel->parent)) {
                return std::monostate{};
            }

            return CreateEntityObjectLocked(interp, registry, rel->parent);
        };

        auto set_parent_body = [id, reg_ptr = &registry](const ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            ECS::EntityID parentId = ECS::INVALID_ENTITY_ID;

            if (!args.empty()) {
                if (std::holds_alternative<double>(args[0])) {
                    parentId = static_cast<ECS::EntityID>(std::get<double>(args[0]));
                } else if (std::holds_alternative<ObSL::ObSLObject *>(args[0])) {
                    auto *parent = std::get<ObSL::ObSLObject *>(args[0]);

                    if (parent) {
                        const auto it = parent->fields.find("id");

                        if (it != parent->fields.end() && std::holds_alternative<double>(it->second)) {
                            parentId = static_cast<ECS::EntityID>(std::get<double>(it->second));
                        }
                    }
                }
            }

            auto apply = [id, parentId](ECS::Registry &reg) {
                if (!reg.IsValid(id)) {
                    return;
                }

                reg.Reparent(id, parentId);
            };

            auto *worker = static_cast<ObSL::ScriptWorker *>(interp->user_data);
            auto *commands = worker ? worker->frame_context<ScriptCommandBuffer>() : nullptr;

            if (commands) {
                commands->push(std::move(apply));
            } else {
                std::unique_lock lock(g_RegistryMutex);
                apply(*reg_ptr);
            }

            return std::monostate{};
        };

        auto get_child_count_body = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
            std::shared_lock lock(g_RegistryMutex);

            if (!registry.IsValid(id)) {
                return 0.0;
            }

            const auto *rel = registry.GetComponent<ECS::Components::RelationshipComponent>(id);

            if (!rel) {
                return 0.0;
            }

            return static_cast<double>(rel->children.size());
        };

        auto find_child_body = [id, &registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.empty() || !std::holds_alternative<std::string>(args[0])) {
                return std::monostate{};
            }

            const auto &childName = std::get<std::string>(args[0]);
            std::shared_lock lock(g_RegistryMutex);

            if (!registry.IsValid(id)) {
                return std::monostate{};
            }

            const auto *rel = registry.GetComponent<ECS::Components::RelationshipComponent>(id);

            if (!rel) {
                return std::monostate{};
            }

            for (const ECS::EntityID childId : rel->children) {
                if (registry.IsValid(childId) && registry.GetEntityName(childId) == childName) {
                    return CreateEntityObjectLocked(interp, registry, childId);
                }
            }

            return std::monostate{};
        };

        obj->fields["SetName"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(set_name_body), "SetName");
        obj->fields["GetName"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(get_name_body), "GetName");
        obj->fields["GetComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(get_comp_body), "GetComponent");
        obj->fields["HasComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(has_comp_body), "HasComponent");
        obj->fields["AddComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(add_comp_body), "AddComponent");
        obj->fields["RemoveComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(remove_comp_body), "RemoveComponent");
        obj->fields["GetComponents"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(get_components_body), "GetComponents");
        obj->fields["Destroy"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(destroy_body), "Destroy");
        obj->fields["SetPersistent"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(set_persistent_body), "SetPersistent");
        obj->fields["IsPersistent"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(is_persistent_body), "IsPersistent");
        obj->fields["AddCustomComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(2, std::move(add_custom_comp), "AddCustomComponent");
        obj->fields["GetCustomComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(get_custom_comp), "GetCustomComponent");
        obj->fields["GetChildren"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(get_children_body), "GetChildren");
        obj->fields["GetParent"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(get_parent_body), "GetParent");
        obj->fields["SetParent"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(set_parent_body), "SetParent");
        obj->fields["GetChildCount"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(get_child_count_body), "GetChildCount");
        obj->fields["Find"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(find_child_body), "Find");

        if (auto *cache = EntityWrapperCache::Get(interpreter)) {
            cache->Store(registry, id, EntityWrapperCache::Kind::Entity, obj);
        }

        return obj;
    }

    ObSL::ObSLObject *CreateEntityObject(ObSL::Interpreter *interpreter, ECS::Registry &registry, const ECS::EntityID id) {
        std::shared_lock lock(g_RegistryMutex);
        return CreateEntityObjectLocked(interpreter, registry, id);
    }

} // namespace Scripting

void Scripting::EngineLib::register_registry_modules(ObSL::Interpreter &interpreter) {
    interpreter.get_global_environment()->define("GetEntity", interpreter.gc.allocate<ObSL::NativeFunction>(1, [reg = m_registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (args.empty() || !std::holds_alternative<double>(args[0])) {
            return std::monostate{};
        }

        const auto id = static_cast<ECS::EntityID>(std::get<double>(args[0]));

        std::shared_lock lock(g_RegistryMutex);

        if (!reg->IsValid(id)) {
            return std::monostate{};
        }

        return CreateEntityObjectLocked(interp, *reg, id);
    }, "GetEntity"));

    interpreter.get_global_environment()->define("Find", interpreter.gc.allocate<ObSL::NativeFunction>(1, [reg = m_registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) {
            return std::monostate{};
        }

        const auto &targetName = std::get<std::string>(args[0]);
        std::shared_lock lock(g_RegistryMutex);

        for (const ECS::EntityID id : reg->GetLivingEntities()) {
            if (reg->GetEntityName(id) == targetName) {
                return CreateEntityObjectLocked(interp, *reg, id);
            }
        }

        return std::monostate{};
    }, "Find"));

    interpreter.get_global_environment()->define("CreateEntity", interpreter.gc.allocate<ObSL::NativeFunction>(1, [reg = m_registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        std::string name = "NewEntity";

        if (!args.empty() && std::holds_alternative<std::string>(args[0])) {
            name = std::get<std::string>(args[0]);
        }

        std::unique_lock lock(g_RegistryMutex);

        const ECS::EntityID id = reg->CreateEntity();
        reg->SetEntityName(id, name);

        return CreateEntityObjectLocked(interp, *reg, id);
    }, "CreateEntity"));

    interpreter.get_global_environment()->define(
            "Instantiate", interpreter.gc.allocate<ObSL::NativeFunction>(1, [reg = m_registry, ctx = m_ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) {
            return std::monostate{};
        }

        const auto &path = std::get<std::string>(args[0]);
        std::unique_lock lock(g_RegistryMutex);

        const ECS::EntityID id = IO::PrefabManager::Instantiate(*reg, *ctx->resources, path);

        if (!reg->IsValid(id)) {
            return std::monostate{};
        }

        return CreateEntityObjectLocked(interp, *reg, id);
    }, "Instantiate"));

    interpreter.get_global_environment()->define(
            "DestroyEntity", interpreter.gc.allocate<ObSL::NativeFunction>(1, [reg = m_registry](const ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (args.empty() || !std::holds_alternative<double>(args[0])) {
            return std::monostate{};
        }

        const auto id = static_cast<ECS::EntityID>(std::get<double>(args[0]));

        auto *worker = static_cast<ObSL::ScriptWorker *>(interp->user_data);
        auto *commands = worker ? worker->frame_context<ScriptCommandBuffer>() : nullptr;

        if (commands) {
            commands->push([id](ECS::Registry &target) { target.DestroyEntity(id); });
        } else {
            std::unique_lock lock(g_RegistryMutex);
            reg->DestroyEntity(id);
        }

        return std::monostate{};
    }, "DestroyEntity"));
}
