#include "EngineLib.h"
#include "EngineLibFactories.h"
#include <string>
#include "ECS/Components/CustomDataComponent.h"


/*
 *todo:
 * gui stuff?
 * wait corutine type thingy (wait for specific time without pausing entire thread type thingy)
 */

// Helper Object
ObSL::ObSLObject *CreateEntityObject(ObSL::Interpreter *interpreter, ECS::Registry &registry, ECS::EntityID id) {
    auto *obj = interpreter->gc.allocate<ObSL::ObSLObject>();
    Scripting::EngineLib::EngineLibFactories::GCProtectGuard guard(interpreter, obj);

    obj->fields["id"] = static_cast<double>(id);
    obj->fields["name"] = registry.GetEntityName(id);

    auto set_name_body = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (!registry.IsValid(id)) return std::monostate{};
        if (!args.empty() && std::holds_alternative<std::string>(args[0])) {
            registry.SetEntityName(id, std::get<std::string>(args[0]));
        }
        return std::monostate{};
    };

    auto get_comp_body = [id, &registry
            ](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (!registry.IsValid(id)) return std::monostate{};
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) return std::monostate{};
        const std::string comp_name = std::get<std::string>(args[0]);

        if (comp_name == "Transform")
            return Scripting::EngineLib::EngineLibFactories::CreateTransformObject(
                interp, registry, id);
        if (comp_name == "PointLight")
            return Scripting::EngineLib::EngineLibFactories::CreatePointLightObject(
                interp, registry, id);
        if (comp_name == "Movement")
            return Scripting::EngineLib::EngineLibFactories::CreateMovementObject(
                interp, registry, id);
        if (comp_name == "MapState")
            return Scripting::EngineLib::EngineLibFactories::CreateMapStateObject(
                interp, registry, id);
        if (comp_name == "DirectionalTexture")
            return Scripting::EngineLib::EngineLibFactories::CreateDirectionalTextureObject(
                interp, registry, id);
        if (comp_name == "BillboardTag")
            return Scripting::EngineLib::EngineLibFactories::CreateBillboardTagObject(
                interp, registry, id);
        if (comp_name == "DestroyTag")
            return Scripting::EngineLib::EngineLibFactories::CreateDestroyTagObject(
                interp, registry, id);

        return std::monostate{};
    };

    auto add_comp_body = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (!registry.IsValid(id)) return std::monostate{};
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) return std::monostate{};
        if (const std::string comp_name = std::get<std::string>(args[0]); comp_name == "DestroyTag") {
            if (!registry.HasComponent<ECS::Components::DestroyTagComponent>(id)) {
                registry.AddComponent<ECS::Components::DestroyTagComponent>(id, ECS::Components::DestroyTagComponent{});
            }
        }
        return std::monostate{};
    };


    // script defined custom components to the ECS
    auto add_custom_comp = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (!registry.IsValid(id)) return std::monostate{};
        // args Component Name , The object/data
        if (args.size() == 2 && std::holds_alternative<std::string>(args[0])) {
            if (!registry.HasComponent<ECS::Components::CustomDataComponent>(id)) {
                registry.AddComponent<ECS::Components::CustomDataComponent>(id, ECS::Components::CustomDataComponent{});
            }
            auto *comp = registry.GetComponent<ECS::Components::CustomDataComponent>(id);
            const std::string compName = std::get<std::string>(args[0]);

            comp->script_components[compName] = args[1];
            return true;
        }
        return false;
    };
    auto get_custom_comp = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (!registry.IsValid(id)) return std::monostate{};
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

void Scripting::EngineLib::EngineLib::register_modules(ObSL::Interpreter &interpreter) {
    register_core_modules(interpreter);
    register_input_modules(interpreter);
    register_camera_modules(interpreter);
    register_map_modules(interpreter);
    register_audio_modules(interpreter);
    register_scene_management_modules(interpreter);
}
