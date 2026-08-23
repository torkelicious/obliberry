#include "Scripting/EngineLib/EngineLib.h"
#include "Scripting/EngineLib/ScriptCommandBuffer.h"
#include "Scripting/EngineLib/EntityWrapperCache.h"
#include <ObSL/ScriptWorker.h>
#include <string>
#include "ECS/Components/BillboardTagComponent.h"
#include "ECS/Components/CustomDataComponent.h"
#include "ECS/Components/DestroyTagComponent.h"
#include "ECS/Components/DirectionalTextureComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/PointLightComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/ParticleEmitterComponent.h"
#include "ECS/Components/PersistentTagComponent.h"
#include "ECS/Components/RelationshipComponent.h"
#include "IO/Loaders/PrefabManager.h"
#include "Scripting/EngineLib/EngineLibFactories.h"
#include <shared_mutex>

// Helper Object
namespace Scripting {

    ObSL::ObSLArray *ChildrenAsArray(ObSL::Interpreter *interp, ECS::Registry &reg, const ECS::EntityID id) {
        auto *arr = interp->gc.allocate<ObSL::ObSLArray>();
        if (reg.IsValid(id)) {
            const ECS::Entity ent(id, &reg);
            for (const auto &children = ent.GetChildren(); const auto child : children) {
                arr->elements.emplace_back(CreateEntityObject(interp, reg, child));
            }
        }
        return arr;
    }

    ObSL::ObSLObject *CreateEntityObject(ObSL::Interpreter *interpreter, ECS::Registry &registry, ECS::EntityID id) {
        // cache hit reuses a previously built wrapper
        // when the entity is still live and belongs to the same registry.
        // names will be updated if such functions are called
        if (auto *cache = EntityWrapperCache::Get(interpreter)) {
            std::shared_lock lk(g_RegistryMutex);
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

        auto set_name_body = [id, reg_ptr = &registry](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.empty() || !std::holds_alternative<std::string>(args[0]))
                return std::monostate{};
            auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
            auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
            auto name = std::get<std::string>(args[0]);
            if (cmd_buf) {
                cmd_buf->push([id, name = std::move(name)](ECS::Registry &reg) {
                    if (!reg.IsValid(id))
                        return;
                    reg.SetEntityName(id, name);
                });
            } else if (reg_ptr) {
                std::unique_lock lock(g_RegistryMutex);
                if (reg_ptr->IsValid(id))
                    reg_ptr->SetEntityName(id, name);
            }
            return std::monostate{};
        };

        auto get_comp_body = [id, &registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            std::shared_lock lock(g_RegistryMutex);
            if (!registry.IsValid(id))
                return std::monostate{};
            if (args.empty() || !std::holds_alternative<std::string>(args[0]))
                return std::monostate{};
            const std::string comp_name = std::get<std::string>(args[0]);

            if (comp_name == "Transform")
                return EngineLibFactories::CreateTransformObject(interp, registry, id);
            if (comp_name == "PointLight")
                return EngineLibFactories::CreatePointLightObject(interp, registry, id);
            if (comp_name == "Movement")
                return EngineLibFactories::CreateMovementObject(interp, registry, id);
            if (comp_name == "MapState")
                return EngineLibFactories::CreateMapStateObject(interp, registry, id);
            if (comp_name == "DirectionalTexture")
                return EngineLibFactories::CreateDirectionalTextureObject(interp, registry, id);
            if (comp_name == "BillboardTag")
                return EngineLibFactories::CreateBillboardTagObject(interp, registry, id);
            if (comp_name == "DestroyTag")
                return EngineLibFactories::CreateDestroyTagObject(interp, registry, id);
            if (comp_name == "ParticleEmitter")
                return EngineLibFactories::CreateParticleEmitterObject(interp, registry, id);

            return std::monostate{};
        };

        auto add_comp_body = [id, reg_ptr = &registry](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.empty() || !std::holds_alternative<std::string>(args[0]))
                return std::monostate{};
            const std::string comp_name = std::get<std::string>(args[0]);
            auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
            if (auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>()) {
                cmd_buf->push([id, comp_name](ECS::Registry &reg) {
                    if (!reg.IsValid(id))
                        return;
                    if (comp_name == "Transform" && !reg.HasComponent<ECS::Components::TransformComponent>(id))
                        reg.AddComponent<ECS::Components::TransformComponent>(id, ECS::Components::TransformComponent{});
                    else if (comp_name == "PointLight" && !reg.HasComponent<ECS::Components::PointLightComponent>(id))
                        reg.AddComponent<ECS::Components::PointLightComponent>(id, ECS::Components::PointLightComponent{});
                    else if (comp_name == "Movement" && !reg.HasComponent<ECS::Components::MovementComponent>(id))
                        reg.AddComponent<ECS::Components::MovementComponent>(id, ECS::Components::MovementComponent{});
                    else if (comp_name == "MapState" && !reg.HasComponent<ECS::Components::MapStateComponent>(id))
                        reg.AddComponent<ECS::Components::MapStateComponent>(id, ECS::Components::MapStateComponent{});
                    else if (comp_name == "DirectionalTexture" && !reg.HasComponent<ECS::Components::DirectionalTextureComponent>(id))
                        reg.AddComponent<ECS::Components::DirectionalTextureComponent>(id, ECS::Components::DirectionalTextureComponent{});
                    else if (comp_name == "BillboardTag" && !reg.HasComponent<ECS::Components::BillboardTagComponent>(id))
                        reg.AddComponent<ECS::Components::BillboardTagComponent>(id, ECS::Components::BillboardTagComponent{});
                    else if (comp_name == "DestroyTag" && !reg.HasComponent<ECS::Components::DestroyTagComponent>(id))
                        reg.AddComponent<ECS::Components::DestroyTagComponent>(id, ECS::Components::DestroyTagComponent{});
                    else if (comp_name == "ParticleEmitter" && !reg.HasComponent<ECS::Components::ParticleEmitterComponent>(id))
                        reg.AddComponent<ECS::Components::ParticleEmitterComponent>(id, ECS::Components::ParticleEmitterComponent{});
                });
            } else if (reg_ptr) {
                std::unique_lock lock(g_RegistryMutex);
                if (!reg_ptr->IsValid(id))
                    return std::monostate{};
                if (comp_name == "Transform" && !reg_ptr->HasComponent<ECS::Components::TransformComponent>(id))
                    reg_ptr->AddComponent<ECS::Components::TransformComponent>(id, ECS::Components::TransformComponent{});
                else if (comp_name == "PointLight" && !reg_ptr->HasComponent<ECS::Components::PointLightComponent>(id))
                    reg_ptr->AddComponent<ECS::Components::PointLightComponent>(id, ECS::Components::PointLightComponent{});
                else if (comp_name == "Movement" && !reg_ptr->HasComponent<ECS::Components::MovementComponent>(id))
                    reg_ptr->AddComponent<ECS::Components::MovementComponent>(id, ECS::Components::MovementComponent{});
                else if (comp_name == "MapState" && !reg_ptr->HasComponent<ECS::Components::MapStateComponent>(id))
                    reg_ptr->AddComponent<ECS::Components::MapStateComponent>(id, ECS::Components::MapStateComponent{});
                else if (comp_name == "DirectionalTexture" && !reg_ptr->HasComponent<ECS::Components::DirectionalTextureComponent>(id))
                    reg_ptr->AddComponent<ECS::Components::DirectionalTextureComponent>(id, ECS::Components::DirectionalTextureComponent{});
                else if (comp_name == "BillboardTag" && !reg_ptr->HasComponent<ECS::Components::BillboardTagComponent>(id))
                    reg_ptr->AddComponent<ECS::Components::BillboardTagComponent>(id, ECS::Components::BillboardTagComponent{});
                else if (comp_name == "DestroyTag" && !reg_ptr->HasComponent<ECS::Components::DestroyTagComponent>(id))
                    reg_ptr->AddComponent<ECS::Components::DestroyTagComponent>(id, ECS::Components::DestroyTagComponent{});
                else if (comp_name == "ParticleEmitter" && !reg_ptr->HasComponent<ECS::Components::ParticleEmitterComponent>(id))
                    reg_ptr->AddComponent<ECS::Components::ParticleEmitterComponent>(id, ECS::Components::ParticleEmitterComponent{});
            }
            return std::monostate{};
        };


        // script defined custom components to the ECS
        auto add_custom_comp = [id, reg_ptr = &registry](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.size() != 2 || !std::holds_alternative<std::string>(args[0]))
                return false;
            auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
            auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
            auto compName = std::get<std::string>(args[0]);
            ObSL::Value val = args[1];
            if (cmd_buf) {
                cmd_buf->push([id, compName = std::move(compName), val = std::move(val)](ECS::Registry &reg) {
                    if (!reg.IsValid(id))
                        return;
                    if (!reg.HasComponent<ECS::Components::CustomDataComponent>(id)) {
                        reg.AddComponent<ECS::Components::CustomDataComponent>(id, ECS::Components::CustomDataComponent{});
                    }
                    auto *comp = reg.GetComponent<ECS::Components::CustomDataComponent>(id);
                    comp->script_components[compName] = val;
                });
            } else if (reg_ptr) {
                std::unique_lock lock(g_RegistryMutex);
                if (!reg_ptr->IsValid(id))
                    return true;
                if (!reg_ptr->HasComponent<ECS::Components::CustomDataComponent>(id)) {
                    reg_ptr->AddComponent<ECS::Components::CustomDataComponent>(id, ECS::Components::CustomDataComponent{});
                }
                if (auto *comp = reg_ptr->GetComponent<ECS::Components::CustomDataComponent>(id))
                    comp->script_components[compName] = val;
            }
            return true;
        };
        auto get_custom_comp = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            std::shared_lock lock(g_RegistryMutex);
            if (!registry.IsValid(id))
                return std::monostate{};
            //  name
            if (args.size() == 1 && std::holds_alternative<std::string>(args[0])) {
                if (auto *comp = registry.GetComponent<ECS::Components::CustomDataComponent>(id)) {
                    if (const auto compName = std::get<std::string>(args[0]); comp->script_components.contains(compName)) {
                        return comp->script_components[compName]; // the script object
                    }
                }
            }
            return std::monostate{};
        };

        // entity utility methods
        auto get_name_body = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
            std::shared_lock lock(g_RegistryMutex);
            if (!registry.IsValid(id))
                return std::monostate{};
            return registry.GetEntityName(id);
        };

        auto has_comp_body = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.empty() || !std::holds_alternative<std::string>(args[0]))
                return false;
            std::shared_lock lock(g_RegistryMutex);
            if (!registry.IsValid(id))
                return false;
            const auto &name = std::get<std::string>(args[0]);
            if (name == "Transform")
                return registry.HasComponent<ECS::Components::TransformComponent>(id);
            if (name == "PointLight")
                return registry.HasComponent<ECS::Components::PointLightComponent>(id);
            if (name == "Movement")
                return registry.HasComponent<ECS::Components::MovementComponent>(id);
            if (name == "MapState")
                return registry.HasComponent<ECS::Components::MapStateComponent>(id);
            if (name == "DirectionalTexture")
                return registry.HasComponent<ECS::Components::DirectionalTextureComponent>(id);
            if (name == "BillboardTag")
                return registry.HasComponent<ECS::Components::BillboardTagComponent>(id);
            if (name == "DestroyTag")
                return registry.HasComponent<ECS::Components::DestroyTagComponent>(id);
            if (name == "ParticleEmitter")
                return registry.HasComponent<ECS::Components::ParticleEmitterComponent>(id);
            return false;
        };

        auto remove_comp_body = [id, reg_ptr = &registry](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.empty() || !std::holds_alternative<std::string>(args[0]))
                return std::monostate{};
            const std::string comp_name = std::get<std::string>(args[0]);
            auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
            if (auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>()) {
                cmd_buf->push([id, comp_name](ECS::Registry &reg) {
                    if (!reg.IsValid(id))
                        return;
                    if (comp_name == "Transform")
                        reg.RemoveComponent<ECS::Components::TransformComponent>(id);
                    else if (comp_name == "PointLight")
                        reg.RemoveComponent<ECS::Components::PointLightComponent>(id);
                    else if (comp_name == "Movement")
                        reg.RemoveComponent<ECS::Components::MovementComponent>(id);
                    else if (comp_name == "MapState")
                        reg.RemoveComponent<ECS::Components::MapStateComponent>(id);
                    else if (comp_name == "DirectionalTexture")
                        reg.RemoveComponent<ECS::Components::DirectionalTextureComponent>(id);
                    else if (comp_name == "BillboardTag")
                        reg.RemoveComponent<ECS::Components::BillboardTagComponent>(id);
                    else if (comp_name == "DestroyTag")
                        reg.RemoveComponent<ECS::Components::DestroyTagComponent>(id);
                    else if (comp_name == "ParticleEmitter")
                        reg.RemoveComponent<ECS::Components::ParticleEmitterComponent>(id);
                });
            } else if (reg_ptr) {
                std::unique_lock lock(g_RegistryMutex);
                if (!reg_ptr->IsValid(id))
                    return std::monostate{};
                if (comp_name == "Transform")
                    reg_ptr->RemoveComponent<ECS::Components::TransformComponent>(id);
                else if (comp_name == "PointLight")
                    reg_ptr->RemoveComponent<ECS::Components::PointLightComponent>(id);
                else if (comp_name == "Movement")
                    reg_ptr->RemoveComponent<ECS::Components::MovementComponent>(id);
                else if (comp_name == "MapState")
                    reg_ptr->RemoveComponent<ECS::Components::MapStateComponent>(id);
                else if (comp_name == "DirectionalTexture")
                    reg_ptr->RemoveComponent<ECS::Components::DirectionalTextureComponent>(id);
                else if (comp_name == "BillboardTag")
                    reg_ptr->RemoveComponent<ECS::Components::BillboardTagComponent>(id);
                if (comp_name == "DestroyTag")
                    reg_ptr->RemoveComponent<ECS::Components::DestroyTagComponent>(id);
                else if (comp_name == "ParticleEmitter")
                    reg_ptr->RemoveComponent<ECS::Components::ParticleEmitterComponent>(id);
            }
            return std::monostate{};
        };

        auto destroy_body = [id, reg_ptr = &registry](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &) -> ObSL::Value {
            auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
            if (auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>()) {
                cmd_buf->push([id](ECS::Registry &reg) { reg.DestroyEntity(id); });
            } else if (reg_ptr) {
                std::unique_lock lock(g_RegistryMutex);
                reg_ptr->DestroyEntity(id);
            }
            return std::monostate{};
        };

        auto get_components_body = [id, &registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
            std::shared_lock lock(g_RegistryMutex);
            if (!registry.IsValid(id))
                return std::monostate{};
            auto *arr = interp->gc.allocate<ObSL::ObSLArray>();
            if (registry.HasComponent<ECS::Components::TransformComponent>(id))
                arr->elements.emplace_back(std::string("Transform"));
            if (registry.HasComponent<ECS::Components::PointLightComponent>(id))
                arr->elements.emplace_back(std::string("PointLight"));
            if (registry.HasComponent<ECS::Components::MovementComponent>(id))
                arr->elements.emplace_back(std::string("Movement"));
            if (registry.HasComponent<ECS::Components::MapStateComponent>(id))
                arr->elements.emplace_back(std::string("MapState"));
            if (registry.HasComponent<ECS::Components::DirectionalTextureComponent>(id))
                arr->elements.emplace_back(std::string("DirectionalTexture"));
            if (registry.HasComponent<ECS::Components::BillboardTagComponent>(id))
                arr->elements.emplace_back(std::string("BillboardTag"));
            if (registry.HasComponent<ECS::Components::DestroyTagComponent>(id))
                arr->elements.emplace_back(std::string("DestroyTag"));
            if (registry.HasComponent<ECS::Components::ParticleEmitterComponent>(id))
                arr->elements.emplace_back(std::string("ParticleEmitter"));
            return arr;
        };

        obj->fields["SetName"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(set_name_body), "SetName");
        obj->fields["GetName"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(get_name_body), "GetName");
        obj->fields["GetComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(get_comp_body), "GetComponent");
        obj->fields["HasComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(has_comp_body), "HasComponent");
        obj->fields["AddComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(add_comp_body), "AddComponent");
        obj->fields["RemoveComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(remove_comp_body), "RemoveComponent");
        obj->fields["GetComponents"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(get_components_body), "GetComponents");
        obj->fields["Destroy"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(destroy_body), "Destroy");

        auto set_persistent_body = [id, reg_ptr = &registry](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.empty() || !std::holds_alternative<bool>(args[0]))
                return std::monostate{};
            const bool shouldPersist = std::get<bool>(args[0]);
            auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
            if (auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>()) {
                cmd_buf->push([id, shouldPersist](ECS::Registry &reg) {
                    if (!reg.IsValid(id))
                        return;
                    if (shouldPersist)
                        reg.AddComponent<ECS::Components::PersistentTagComponent>(id);
                    else
                        reg.RemoveComponent<ECS::Components::PersistentTagComponent>(id);
                });
            } else if (reg_ptr) {
                std::unique_lock lock(g_RegistryMutex);
                if (!reg_ptr->IsValid(id))
                    return std::monostate{};
                if (shouldPersist)
                    reg_ptr->AddComponent<ECS::Components::PersistentTagComponent>(id);
                else
                    reg_ptr->RemoveComponent<ECS::Components::PersistentTagComponent>(id);
            }
            return std::monostate{};
        };

        auto is_persistent_body = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
            std::shared_lock lock(g_RegistryMutex);
            if (!registry.IsValid(id))
                return false;
            return registry.HasComponent<ECS::Components::PersistentTagComponent>(id);
        };

        obj->fields["SetPersistent"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(set_persistent_body), "SetPersistent");
        obj->fields["IsPersistent"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(is_persistent_body), "IsPersistent");
        obj->fields["AddCustomComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(2, std::move(add_custom_comp), "AddCustomComponent");
        obj->fields["GetCustomComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(get_custom_comp), "GetCustomComponent");

        // hierarchy methods

        auto get_children_body = [id, &registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
            std::shared_lock lock(g_RegistryMutex);
            return ChildrenAsArray(interp, registry, id);
        };

        auto get_parent_body = [id, &registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
            std::shared_lock lock(g_RegistryMutex);
            if (!registry.IsValid(id))
                return std::monostate{};
            auto *rel = registry.GetComponent<ECS::Components::RelationshipComponent>(id);
            if (!rel || rel->parent == ECS::INVALID_ENTITY_ID || !registry.IsValid(rel->parent))
                return std::monostate{};
            return CreateEntityObject(interp, registry, rel->parent);
        };

        auto set_parent_body = [id, reg_ptr = &registry](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            ECS::EntityID parentId = 0;
            if (!args.empty()) {
                if (std::holds_alternative<double>(args[0])) {
                    parentId = static_cast<ECS::EntityID>(std::get<double>(args[0]));
                } else if (std::holds_alternative<ObSL::ObSLObject *>(args[0])) {
                    auto *obj = std::get<ObSL::ObSLObject *>(args[0]);
                    if (auto it = obj->fields.find("id"); it != obj->fields.end() && std::holds_alternative<double>(it->second))
                        parentId = static_cast<ECS::EntityID>(std::get<double>(it->second));
                }
            }
            auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
            if (auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>()) {
                cmd_buf->push([id, parentId](ECS::Registry &reg) {
                    if (!reg.IsValid(id))
                        return;
                    reg.Reparent(id, parentId);
                });
            } else if (reg_ptr) {
                std::unique_lock lock(g_RegistryMutex);
                if (reg_ptr->IsValid(id))
                    reg_ptr->Reparent(id, parentId);
            }
            return std::monostate{};
        };

        auto get_child_count_body = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
            std::shared_lock lock(g_RegistryMutex);
            if (!registry.IsValid(id))
                return 0.0;
            auto *rel = registry.GetComponent<ECS::Components::RelationshipComponent>(id);
            if (!rel)
                return 0.0;
            return static_cast<double>(rel->children.size());
        };

        auto find_child_body = [id, &registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.empty() || !std::holds_alternative<std::string>(args[0]))
                return std::monostate{};
            const auto &childName = std::get<std::string>(args[0]);
            std::shared_lock lock(g_RegistryMutex);
            if (!registry.IsValid(id))
                return std::monostate{};
            auto *rel = registry.GetComponent<ECS::Components::RelationshipComponent>(id);
            if (!rel)
                return std::monostate{};
            for (const ECS::EntityID childId : rel->children) {
                if (registry.IsValid(childId) && registry.GetEntityName(childId) == childName)
                    return CreateEntityObject(interp, registry, childId);
            }
            return std::monostate{};
        };

        obj->fields["GetChildren"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(get_children_body), "GetChildren");
        obj->fields["GetParent"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(get_parent_body), "GetParent");
        obj->fields["SetParent"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(set_parent_body), "SetParent");
        obj->fields["GetChildCount"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(get_child_count_body), "GetChildCount");
        obj->fields["Find"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(find_child_body), "Find");

        if (auto *cache = EntityWrapperCache::Get(interpreter))
            cache->Store(registry, id, EntityWrapperCache::Kind::Entity, obj);

        return obj;
    }
} // namespace Scripting

void Scripting::EngineLib::register_registry_modules(ObSL::Interpreter &interpreter) {


    interpreter.get_global_environment()->define("GetEntity", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                      1,
                                                                      [reg = m_registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                                                          if (args.empty() || !std::holds_alternative<double>(args[0]))
                                                                              return std::monostate{};

                                                                          const auto id = static_cast<ECS::EntityID>(std::get<double>(args[0]));

                                                                          // Read-only
                                                                          std::shared_lock lock(g_RegistryMutex);
                                                                          return CreateEntityObject(interp, *reg, id);
                                                                      },
                                                                      "GetEntity"));


    interpreter.get_global_environment()->define("Find", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                 1,
                                                                 [reg = m_registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                                                     if (args.empty() || !std::holds_alternative<std::string>(args[0]))
                                                                         return std::monostate{};
                                                                     const auto target_name = std::get<std::string>(args[0]);

                                                                     std::shared_lock lock(g_RegistryMutex);
                                                                     for (const ECS::EntityID id : reg->GetLivingEntities()) {
                                                                         if (reg->GetEntityName(id) == target_name)
                                                                             return CreateEntityObject(interp, *reg, id);
                                                                     }
                                                                     return std::monostate{};
                                                                 },
                                                                 "Find"));

    interpreter.get_global_environment()->define("CreateEntity", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                         1,
                                                                         [reg = m_registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                                                             std::string name = "NewEntity";
                                                                             if (!args.empty() && std::holds_alternative<std::string>(args[0])) {
                                                                                 name = std::get<std::string>(args[0]);
                                                                             }
                                                                             auto *worker = interp->user_data ? static_cast<ObSL::ScriptWorker *>(interp->user_data) : nullptr;
                                                                             auto *cmd_buf = worker ? worker->frame_context<ScriptCommandBuffer>() : nullptr;
                                                                             std::unique_lock lock(g_RegistryMutex);
                                                                             const ECS::EntityID new_id = reg->CreateEntity();
                                                                             reg->SetEntityName(new_id, name);
                                                                             return CreateEntityObject(interp, *reg, new_id);
                                                                         },
                                                                         "CreateEntity"));

    interpreter.get_global_environment()->define("Instantiate", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                        1,
                                                                        [reg = m_registry, ctx = m_ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                                                            if (args.empty() || !std::holds_alternative<std::string>(args[0]))
                                                                                return std::monostate{};
                                                                            const std::string prefab_path = std::get<std::string>(args[0]);
                                                                            auto *worker = interp->user_data ? static_cast<ObSL::ScriptWorker *>(interp->user_data) : nullptr;
                                                                            auto *cmd_buf = worker ? worker->frame_context<ScriptCommandBuffer>() : nullptr;
                                                                            std::unique_lock lock(g_RegistryMutex);
                                                                            const ECS::EntityID new_id = IO::PrefabManager::Instantiate(*reg, *ctx->resources, prefab_path);
                                                                            if (new_id == 0)
                                                                                return std::monostate{};
                                                                            return CreateEntityObject(interp, *reg, new_id);
                                                                        },
                                                                        "Instantiate"));


    interpreter.get_global_environment()->define("DestroyEntity", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                          1,
                                                                          [reg = m_registry](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                                                              if (args.empty() || !std::holds_alternative<double>(args[0]))
                                                                                  return std::monostate{};
                                                                              const auto id = static_cast<ECS::EntityID>(std::get<double>(args[0]));
                                                                              auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
                                                                              if (auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>()) {
                                                                                  cmd_buf->push([id](ECS::Registry &reg) { reg.DestroyEntity(id); });
                                                                              } else if (reg) {
                                                                                  std::unique_lock lock(g_RegistryMutex);
                                                                                  reg->DestroyEntity(id);
                                                                              }
                                                                              return std::monostate{};
                                                                          },
                                                                          "DestroyEntity"));
}
