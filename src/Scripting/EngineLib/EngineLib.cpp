#include "EngineLib.h"
#include "EngineLibFactories.h"
#include "Scripting/EngineLib/ScriptCommandBuffer.h"
#include "Scripting/ObSLCore/ScriptWorker.h"
#include <string>
#include "ECS/Components/CustomDataComponent.h"
#include "ECS/Components/DestroyTagComponent.h"


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

        auto set_name_body = [id](const ObSL::Interpreter *interpreter,
                                  const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.empty() || !std::holds_alternative<std::string>(args[0])) return std::monostate{};
            auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
            auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
            std::string name = std::get<std::string>(args[0]);
            cmd_buf->push([id, name = std::move(name)](ECS::Registry &reg) {
                if (!reg.IsValid(id)) return;
                reg.SetEntityName(id, name);
            });
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

        auto add_comp_body = [id](const ObSL::Interpreter *interpreter,
                                  const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.empty() || !std::holds_alternative<std::string>(args[0])) return std::monostate{};
            if (std::get<std::string>(args[0]) != "DestroyTag") return std::monostate{};
            auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
            auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
            cmd_buf->push([id](ECS::Registry &reg) {
                if (!reg.IsValid(id)) return;
                if (!reg.HasComponent<ECS::Components::DestroyTagComponent>(id)) {
                    reg.AddComponent<ECS::Components::DestroyTagComponent>(
                        id, ECS::Components::DestroyTagComponent{});
                }
            });
            return std::monostate{};
        };


        // script defined custom components to the ECS
        auto add_custom_comp = [id
                ](const ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (args.size() != 2 || !std::holds_alternative<std::string>(args[0])) return false;
            auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
            auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
            std::string compName = std::get<std::string>(args[0]);
            ObSL::Value val = args[1];
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

        obj->fields["SetName"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(set_name_body), "SetName");
        obj->fields["GetComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(
            1, std::move(get_comp_body), "GetComponent");
        obj->fields["AddComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(
            1, std::move(add_comp_body), "AddComponent");
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
}
