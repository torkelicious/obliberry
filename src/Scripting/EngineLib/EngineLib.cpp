#include "EngineLib.h"
#include "EngineLibFactories.h"
#include <string>
#include "Renderer/Camera.h"
#include "Scripting/ObSLCore/Interpreter/Interpreter.h"

// Entity script object
ObSL::ObSLObject *CreateEntityObject(ObSL::Interpreter *interpreter, Registry &registry, EntityID id) {
    auto *obj = interpreter->gc.allocate<ObSL::ObSLObject>();
    interpreter->gc_protect_stack.push_back(obj);
    // basic properties
    obj->fields["id"] = static_cast<double>(id);
    obj->fields["name"] = registry.GetEntityName(id);

    // SetName
    auto set_name_body = [id, &registry](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (!args.empty() && std::holds_alternative<std::string>(args[0])) {
            registry.SetEntityName(id, std::get<std::string>(args[0]));
        }
        return std::monostate{};
    };
    obj->fields["SetName"] = interpreter->gc.allocate<ObSL::NativeFunction>(
        1, std::move(set_name_body), "SetName"
    );

    // GetComponent
    auto get_comp_body = [id, &registry
            ](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) {
            return std::monostate{};
        }

        const auto comp_name = std::get<std::string>(args[0]);

        if (comp_name == "Transform" && registry.HasComponent<TransformComponent>(id)) {
            return EngineLibFactories::CreateTransformObject(interp, registry, id);
        }
        if (comp_name == "PointLight" && registry.HasComponent<PointLightComponent>(id)) {
            return EngineLibFactories::CreatePointLightObject(interp, registry, id);
        }
        if (comp_name == "Movement" && registry.HasComponent<MovementComponent>(id)) {
            return EngineLibFactories::CreateMovementObject(interp, registry, id);
        }
        if (comp_name == "MapState" && registry.HasComponent<MapStateComponent>(id)) {
            return EngineLibFactories::CreateMapStateObject(interp, registry, id);
        }
        if (comp_name == "DirectionalTexture" && registry.HasComponent<DirectionalTextureComponent>(id)) {
            return EngineLibFactories::CreateDirectionalTextureObject(interp, registry, id);
        }
        if (comp_name == "PlayerInput" && registry.HasComponent<PlayerInputComponent>(id)) {
            return EngineLibFactories::CreatePlayerInputObject(interp, registry, id);
        }
        if (comp_name == "BillboardTag" && registry.HasComponent<BillboardTagComponent>(id)) {
            return EngineLibFactories::CreateBillboardTagObject(interp, registry, id);
        }
        if (comp_name == "DestroyTag" && registry.HasComponent<DestroyTagComponent>(id)) {
            return EngineLibFactories::CreateDestroyTagObject(interp, registry, id);
        }

        return std::monostate{};
    };

    obj->fields["GetComponent"] = interpreter->gc.allocate<ObSL::NativeFunction>(
        1, std::move(get_comp_body), "GetComponent"
    );

    interpreter->gc_protect_stack.pop_back();
    return obj;
}


void EngineLib::register_modules(ObSL::Interpreter &interpreter) {
    Registry *reg = m_registry;

    interpreter.get_global_environment()->define(
        "Find",
        interpreter.gc.allocate<ObSL::NativeFunction>(
            1,
            [reg](ObSL::Interpreter *interp,
                  const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (args.empty() || !std::holds_alternative<std::string>(
                        args[0])) {
                    return std::monostate{};
                }

                const auto target_name = std::get<std::string>(args[0]);

                for (const EntityID id: reg->GetLivingEntities()) {
                    if (reg->GetEntityName(id) == target_name) {
                        return CreateEntityObject(interp, *reg, id);
                    }
                }
                return std::monostate{};
            },
            "Find"
        ));
}
