#include "EngineLib.h"
#include "EngineLibFactories.h"
#include "Scripting/EngineLib/ScriptCommandBuffer.h"
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


/*
 *todo:
 * gui stuff?
 * wait corutine type thingy (wait for specific time without pausing entire thread type thingy)
 */

// Helper Object
namespace Scripting {
    ObSL::ObSLObject *CreateEntityObject(ObSL::Interpreter *interpreter, ECS::Registry &registry, ECS::EntityID id) {
        auto *obj = interpreter->gc.allocate<ObSL::ObSLObject>();
        EngineLibFactories::GCProtectGuard guard(interpreter, obj);

        obj->fields["id"] = static_cast<double>(id);
        obj->fields["name"] = registry.GetEntityName(id);

        auto set_name_body = [id, reg_ptr = &registry](const ObSL::Interpreter *interpreter,
                                                       const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.empty() || !std::holds_alternative<std::string>(args[0])) return std::monostate{};
            auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
            auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
            std::string name = std::get<std::string>(args[0]);
            if (cmd_buf) {
                cmd_buf->push([id, name = std::move(name)](ECS::Registry &reg) {
                    if (!reg.IsValid(id)) return;
                    reg.SetEntityName(id, name);
                });
            } else if (reg_ptr) {
                std::unique_lock lock(g_RegistryMutex);
                if (reg_ptr->IsValid(id))
                    reg_ptr->SetEntityName(id, name);
            }
            return std::monostate{};
        };

        auto get_comp_body = [id, &registry
                ](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            std::shared_lock lock(g_RegistryMutex);
            if (!registry.IsValid(id)) return std::monostate{};
            if (args.empty() || !std::holds_alternative<std::string>(args[0])) return std::monostate{};
            const std::string comp_name = std::get<std::string>(args[0]);

            if (comp_name == "Transform")
                return EngineLibFactories::CreateTransformObject(
                    interp, registry, id);
            if (comp_name == "PointLight")
                return EngineLibFactories::CreatePointLightObject(
                    interp, registry, id);
            if (comp_name == "Movement")
                return EngineLibFactories::CreateMovementObject(
                    interp, registry, id);
            if (comp_name == "MapState")
                return EngineLibFactories::CreateMapStateObject(
                    interp, registry, id);
            if (comp_name == "DirectionalTexture")
                return EngineLibFactories::CreateDirectionalTextureObject(
                    interp, registry, id);
            if (comp_name == "BillboardTag")
                return EngineLibFactories::CreateBillboardTagObject(
                    interp, registry, id);
            if (comp_name == "DestroyTag")
                return EngineLibFactories::CreateDestroyTagObject(
                    interp, registry, id);

            return std::monostate{};
        };

        auto add_comp_body = [id, reg_ptr = &registry](const ObSL::Interpreter *interpreter,
                                                       const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.empty() || !std::holds_alternative<std::string>(args[0])) return std::monostate{};
            const std::string comp_name = std::get<std::string>(args[0]);
            auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
            auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
            if (cmd_buf) {
                cmd_buf->push([id, comp_name](ECS::Registry &reg) {
                    if (!reg.IsValid(id)) return;
                    if (comp_name == "Transform" && !reg.HasComponent<ECS::Components::TransformComponent>(id))
                        reg.AddComponent<
                            ECS::Components::TransformComponent>(id, ECS::Components::TransformComponent{});
                    else if (comp_name == "PointLight" && !reg.HasComponent<ECS::Components::PointLightComponent>(id))
                        reg.AddComponent<ECS::Components::PointLightComponent>(
                            id, ECS::Components::PointLightComponent{});
                    else if (comp_name == "Movement" && !reg.HasComponent<ECS::Components::MovementComponent>(id))
                        reg.AddComponent<ECS::Components::MovementComponent>(id, ECS::Components::MovementComponent{});
                    else if (comp_name == "MapState" && !reg.HasComponent<ECS::Components::MapStateComponent>(id))
                        reg.AddComponent<ECS::Components::MapStateComponent>(id, ECS::Components::MapStateComponent{});
                    else if (comp_name == "DirectionalTexture" && !reg.HasComponent<
                                 ECS::Components::DirectionalTextureComponent>(id))
                        reg.AddComponent<ECS::Components::DirectionalTextureComponent>(
                            id, ECS::Components::DirectionalTextureComponent{});
                    else if (comp_name == "BillboardTag" && !reg.HasComponent<
                                 ECS::Components::BillboardTagComponent>(id))
                        reg.AddComponent<ECS::Components::BillboardTagComponent>(
                            id, ECS::Components::BillboardTagComponent{});
                    else if (comp_name == "DestroyTag" && !reg.HasComponent<ECS::Components::DestroyTagComponent>(id))
                        reg.AddComponent<ECS::Components::DestroyTagComponent>(
                            id, ECS::Components::DestroyTagComponent{});
                });
            } else if (reg_ptr) {
                std::unique_lock lock(g_RegistryMutex);
                if (!reg_ptr->IsValid(id)) return std::monostate{};
                if (comp_name == "Transform" && !reg_ptr->HasComponent<ECS::Components::TransformComponent>(id))
                    reg_ptr->AddComponent<ECS::Components::TransformComponent>(
                        id, ECS::Components::TransformComponent{});
                else if (comp_name == "PointLight" && !reg_ptr->HasComponent<ECS::Components::PointLightComponent>(id))
                    reg_ptr->AddComponent<ECS::Components::PointLightComponent>(
                        id, ECS::Components::PointLightComponent{});
                else if (comp_name == "Movement" && !reg_ptr->HasComponent<ECS::Components::MovementComponent>(id))
                    reg_ptr->AddComponent<ECS::Components::MovementComponent>(id, ECS::Components::MovementComponent{});
                else if (comp_name == "MapState" && !reg_ptr->HasComponent<ECS::Components::MapStateComponent>(id))
                    reg_ptr->AddComponent<ECS::Components::MapStateComponent>(id, ECS::Components::MapStateComponent{});
                else if (comp_name == "DirectionalTexture" && !reg_ptr->HasComponent<
                             ECS::Components::DirectionalTextureComponent>(id))
                    reg_ptr->AddComponent<ECS::Components::DirectionalTextureComponent>(
                        id, ECS::Components::DirectionalTextureComponent{});
                else if (comp_name == "BillboardTag" && !reg_ptr->HasComponent<
                             ECS::Components::BillboardTagComponent>(id))
                    reg_ptr->AddComponent<ECS::Components::BillboardTagComponent>(
                        id, ECS::Components::BillboardTagComponent{});
                else if (comp_name == "DestroyTag" && !reg_ptr->HasComponent<ECS::Components::DestroyTagComponent>(id))
                    reg_ptr->AddComponent<ECS::Components::DestroyTagComponent>(
                        id, ECS::Components::DestroyTagComponent{});
            }
            return std::monostate{};
        };


        // script defined custom components to the ECS
        auto add_custom_comp = [id, reg_ptr = &registry
                ](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.size() != 2 || !std::holds_alternative<std::string>(args[0])) return false;
            auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
            auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
            std::string compName = std::get<std::string>(args[0]);
            ObSL::Value val = args[1];
            if (cmd_buf) {
                cmd_buf->push(
                    [id, compName = std::move(compName), val = std::move(val)](ECS::Registry &reg) {
                        if (!reg.IsValid(id)) return;
                        if (!reg.HasComponent<ECS::Components::CustomDataComponent>(id)) {
                            reg.AddComponent<ECS::Components::CustomDataComponent>(
                                id, ECS::Components::CustomDataComponent{});
                        }
                        auto *comp = reg.GetComponent<ECS::Components::CustomDataComponent>(id);
                        comp->script_components[compName] = val;
                    });
            } else if (reg_ptr) {
                std::unique_lock lock(g_RegistryMutex);
                if (!reg_ptr->IsValid(id)) return true;
                if (!reg_ptr->HasComponent<ECS::Components::CustomDataComponent>(id)) {
                    reg_ptr->AddComponent<ECS::Components::CustomDataComponent>(
                        id, ECS::Components::CustomDataComponent{});
                }
                auto *comp = reg_ptr->GetComponent<ECS::Components::CustomDataComponent>(id);
                if (comp) comp->script_components[compName] = val;
            }
            return true;
        };
        auto get_custom_comp = [id, &registry
                ](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            std::shared_lock lock(g_RegistryMutex);
            if (!registry.IsValid(id)) return std::monostate{};
            //  name
            if (args.size() == 1 && std::holds_alternative<std::string>(args[0])) {
                if (auto *comp = registry.GetComponent<ECS::Components::CustomDataComponent>(id)) {
                    if (const auto compName = std::get<std::string>(args[0]); comp->script_components.
                        contains(compName)) {
                        return comp->script_components[compName]; // the script object
                    }
                }
            }
            return std::monostate{};
        };

        // entity utility methods
        auto get_name_body = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
            std::shared_lock lock(g_RegistryMutex);
            if (!registry.IsValid(id)) return std::monostate{};
            return registry.GetEntityName(id);
        };

        auto has_comp_body = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.empty() || !std::holds_alternative<std::string>(args[0])) return false;
            std::shared_lock lock(g_RegistryMutex);
            if (!registry.IsValid(id)) return false;
            const auto &name = std::get<std::string>(args[0]);
            if (name == "Transform") return registry.HasComponent<ECS::Components::TransformComponent>(id);
            if (name == "PointLight") return registry.HasComponent<ECS::Components::PointLightComponent>(id);
            if (name == "Movement") return registry.HasComponent<ECS::Components::MovementComponent>(id);
            if (name == "MapState") return registry.HasComponent<ECS::Components::MapStateComponent>(id);
            if (name == "DirectionalTexture") return registry.HasComponent<
                ECS::Components::DirectionalTextureComponent>(id);
            if (name == "BillboardTag") return registry.HasComponent<ECS::Components::BillboardTagComponent>(id);
            if (name == "DestroyTag") return registry.HasComponent<ECS::Components::DestroyTagComponent>(id);
            return false;
        };

        auto remove_comp_body = [id, reg_ptr = &registry](const ObSL::Interpreter *interpreter,
                                                          const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.empty() || !std::holds_alternative<std::string>(args[0])) return std::monostate{};
            const std::string comp_name = std::get<std::string>(args[0]);
            auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
            auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
            if (cmd_buf) {
                cmd_buf->push([id, comp_name](ECS::Registry &reg) {
                    if (!reg.IsValid(id)) return;
                    if (comp_name == "Transform") reg.RemoveComponent<ECS::Components::TransformComponent>(id);
                    else if (comp_name == "PointLight") reg.RemoveComponent<ECS::Components::PointLightComponent>(id);
                    else if (comp_name == "Movement") reg.RemoveComponent<ECS::Components::MovementComponent>(id);
                    else if (comp_name == "MapState") reg.RemoveComponent<ECS::Components::MapStateComponent>(id);
                    else if (comp_name == "DirectionalTexture") reg.RemoveComponent<
                        ECS::Components::DirectionalTextureComponent>(id);
                    else if (comp_name == "BillboardTag") reg.RemoveComponent<
                        ECS::Components::BillboardTagComponent>(id);
                    else if (comp_name == "DestroyTag") reg.RemoveComponent<ECS::Components::DestroyTagComponent>(id);
                });
            } else if (reg_ptr) {
                std::unique_lock lock(g_RegistryMutex);
                if (!reg_ptr->IsValid(id)) return std::monostate{};
                if (comp_name == "Transform") reg_ptr->RemoveComponent<ECS::Components::TransformComponent>(id);
                else if (comp_name == "PointLight") reg_ptr->RemoveComponent<ECS::Components::PointLightComponent>(id);
                else if (comp_name == "Movement") reg_ptr->RemoveComponent<ECS::Components::MovementComponent>(id);
                else if (comp_name == "MapState") reg_ptr->RemoveComponent<ECS::Components::MapStateComponent>(id);
                else if (comp_name == "DirectionalTexture") reg_ptr->RemoveComponent<
                    ECS::Components::DirectionalTextureComponent>(id);
                else if (comp_name == "BillboardTag") reg_ptr->RemoveComponent<
                    ECS::Components::BillboardTagComponent>(id);
                else if (comp_name == "DestroyTag") reg_ptr->RemoveComponent<ECS::Components::DestroyTagComponent>(id);
            }
            return std::monostate{};
        };

        auto destroy_body = [id, reg_ptr = &registry](const ObSL::Interpreter *interpreter,
                                                      const std::vector<ObSL::Value> &) -> ObSL::Value {
            auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
            auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
            if (cmd_buf) {
                cmd_buf->push([id](ECS::Registry &reg) {
                    reg.DestroyEntity(id);
                });
            } else if (reg_ptr) {
                std::unique_lock lock(g_RegistryMutex);
                reg_ptr->DestroyEntity(id);
            }
            return std::monostate{};
        };

        auto get_components_body = [id, &registry](ObSL::Interpreter *interp,
                                                   const std::vector<ObSL::Value> &) -> ObSL::Value {
            std::shared_lock lock(g_RegistryMutex);
            if (!registry.IsValid(id)) return std::monostate{};
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
            return arr;
        };

        obj->fields["SetName"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(set_name_body), "SetName");
        obj->fields["GetName"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(get_name_body), "GetName");
        obj->fields["GetComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(
            1, std::move(get_comp_body), "GetComponent");
        obj->fields["HasComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(
            1, std::move(has_comp_body), "HasComponent");
        obj->fields["AddComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(
            1, std::move(add_comp_body), "AddComponent");
        obj->fields["RemoveComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(
            1, std::move(remove_comp_body), "RemoveComponent");
        obj->fields["GetComponents"] = interpreter->gc.allocate<ObSL::NativeFunction>(
            0, std::move(get_components_body), "GetComponents");
        obj->fields["Destroy"] = interpreter->gc.allocate<ObSL::NativeFunction>(0, std::move(destroy_body), "Destroy");
        obj->fields["AddCustomComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(
            2, std::move(add_custom_comp), "AddCustomComponent");
        obj->fields["GetCustomComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(
            1, std::move(get_custom_comp), "GetCustomComponent");
        return obj;
    }
}

void Scripting::EngineLib::register_modules(ObSL::Interpreter &interpreter) {
    register_core_modules(interpreter);
    register_input_modules(interpreter);
    register_camera_modules(interpreter);
    register_map_modules(interpreter);
    register_audio_modules(interpreter);
    register_scene_management_modules(interpreter);
    register_time_modules(interpreter);
    register_gui_modules(interpreter);
}
