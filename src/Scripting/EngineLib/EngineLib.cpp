#include "EngineLib.h"
#include "EngineLibFactories.h"
#include <string>


/*
 *todo:
 * Audio (wip)
 * Scene Management & general I/O (wip)
 * gui stuff?
 * user defined components? idk
 * wait corutine type thingy (wait for specific time without pausing entire thread type thingy)
 */


// Helper Object
ObSL::ObSLObject *CreateEntityObject(ObSL::Interpreter *interpreter, Registry &registry, EntityID id) {
    auto *obj = interpreter->gc.allocate<ObSL::ObSLObject>();
    EngineLibFactories::GCProtectGuard guard(interpreter, obj);

    obj->fields["id"] = static_cast<double>(id);
    obj->fields["name"] = registry.GetEntityName(id);

    auto set_name_body = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (!args.empty() && std::holds_alternative<std::string>(args[0])) {
            registry.SetEntityName(id, std::get<std::string>(args[0]));
        }
        return std::monostate{};
    };

    auto get_comp_body = [id, &registry
            ](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) return std::monostate{};
        const std::string comp_name = std::get<std::string>(args[0]);

        if (comp_name == "Transform") return EngineLibFactories::CreateTransformObject(interp, registry, id);
        if (comp_name == "PointLight") return EngineLibFactories::CreatePointLightObject(interp, registry, id);
        if (comp_name == "Movement") return EngineLibFactories::CreateMovementObject(interp, registry, id);
        if (comp_name == "MapState") return EngineLibFactories::CreateMapStateObject(interp, registry, id);
        if (comp_name == "DirectionalTexture")
            return EngineLibFactories::CreateDirectionalTextureObject(
                interp, registry, id);
        if (comp_name == "BillboardTag") return EngineLibFactories::CreateBillboardTagObject(interp, registry, id);
        if (comp_name == "DestroyTag") return EngineLibFactories::CreateDestroyTagObject(interp, registry, id);

        return std::monostate{};
    };

    auto add_comp_body = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) return std::monostate{};
        if (const std::string comp_name = std::get<std::string>(args[0]); comp_name == "DestroyTag") {
            if (!registry.HasComponent<DestroyTagComponent>(id)) {
                registry.AddComponent<DestroyTagComponent>(id, DestroyTagComponent{});
            }
        }
        return std::monostate{};
    };

    obj->fields["SetName"] = interpreter->gc.allocate<ObSL::NativeFunction>(1, std::move(set_name_body), "SetName");
    obj->fields["GetComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(
        1, std::move(get_comp_body), "GetComponent");
    obj->fields["AddComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(
        1, std::move(add_comp_body), "AddComponent");
    return obj;
}

void EngineLib::register_modules(ObSL::Interpreter &interpreter) {
    register_core_modules(interpreter);
    register_input_modules(interpreter);
    register_camera_modules(interpreter);
    register_map_modules(interpreter);
    register_audio_modules(interpreter);
    register_scene_management_modules(interpreter);
}
