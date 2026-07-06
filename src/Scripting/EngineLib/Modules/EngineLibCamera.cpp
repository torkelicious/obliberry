#include "../EngineLib.h"
#include <mutex>
#include "Rendering/Camera.h"
#include <ObSL/Interpreter.h>
#include <ObSL/Natives.h>

namespace {
    std::mutex s_CameraMutex;
}

void Scripting::EngineLib::register_camera_modules(ObSL::Interpreter &interpreter) {
    interpreter.get_global_environment()->define(
        "Camera_GetPosition", interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [ctx = m_ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
                auto *obj = interp->gc.allocate<ObSL::ObSLObject>();
                if (ctx && ctx->camera) {
                    std::lock_guard lock(s_CameraMutex);
                    obj->fields["x"] = static_cast<double>(ctx->camera->Position.x);
                    obj->fields["y"] = static_cast<double>(ctx->camera->Position.y);
                    obj->fields["z"] = static_cast<double>(ctx->camera->Position.z);
                } else {
                    obj->fields["x"] = 0.0;
                    obj->fields["y"] = 0.0;
                    obj->fields["z"] = 0.0;
                }
                return obj;
            }, "Camera_GetPosition"));

    interpreter.get_global_environment()->define(
        "Camera_SetPosition", interpreter.gc.allocate<ObSL::NativeFunction>(
            3,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (ctx && ctx->camera && args.size() >= 3
                    && std::holds_alternative<double>(args[0])
                    && std::holds_alternative<double>(args[1])
                    && std::holds_alternative<double>(args[2])) {
                    std::lock_guard lock(s_CameraMutex);
                    ctx->camera->Position.x = static_cast<float>(std::get<double>(args[0]));
                    ctx->camera->Position.y = static_cast<float>(std::get<double>(args[1]));
                    ctx->camera->Position.z = static_cast<float>(std::get<double>(args[2]));
                    return true;
                }
                return false;
            }, "Camera_SetPosition"));

    interpreter.get_global_environment()->define(
        "Camera_Move", interpreter.gc.allocate<ObSL::NativeFunction>(
            3,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (ctx && ctx->camera && args.size() >= 3
                    && std::holds_alternative<double>(args[0])
                    && std::holds_alternative<double>(args[1])
                    && std::holds_alternative<double>(args[2])) {
                    std::lock_guard lock(s_CameraMutex);
                    ctx->camera->Position.x += static_cast<float>(std::get<double>(args[0]));
                    ctx->camera->Position.y += static_cast<float>(std::get<double>(args[1]));
                    ctx->camera->Position.z += static_cast<float>(std::get<double>(args[2]));
                    return true;
                }
                return false;
            }, "Camera_Move"));

    interpreter.get_global_environment()->define(
        "Camera_PanScreenSpace", interpreter.gc.allocate<ObSL::NativeFunction>(
            2,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (ctx && ctx->camera && args.size() >= 2
                    && std::holds_alternative<double>(args[0])
                    && std::holds_alternative<double>(args[1])) {
                    std::lock_guard lock(s_CameraMutex);
                    glm::vec2 screenPan(
                        static_cast<float>(std::get<double>(args[0])),
                        static_cast<float>(std::get<double>(args[1]))
                    );

                    constexpr float VERTICAL_COMPENSATION = 1.4f;
                    screenPan.y *= VERTICAL_COMPENSATION;

                    const glm::mat4 invRot = glm::inverse(ctx->camera->GetRotation());
                    const glm::vec4 worldPan = invRot * glm::vec4(screenPan.x, screenPan.y, 0.0f, 0.0f);

                    ctx->camera->Position += glm::vec3(worldPan.x, worldPan.y, 0.0f) * (1.0f / ctx->camera->Zoom);
                    return true;
                }
                return false;
            }, "Camera_PanScreenSpace"));

    interpreter.get_global_environment()->define(
        "Camera_GetZoom", interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                if (ctx && ctx->camera) {
                    std::lock_guard lock(s_CameraMutex);
                    return ctx->camera->Zoom;
                }
                return 0.0;
            }, "Camera_GetZoom"));

    interpreter.get_global_environment()->define(
        "Camera_SetZoom", interpreter.gc.allocate<ObSL::NativeFunction>(
            1,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (ctx && ctx->camera && !args.empty() && std::holds_alternative<double>(args[0])) {
                    std::lock_guard lock(s_CameraMutex);
                    ctx->camera->Zoom = static_cast<float>(std::get<double>(args[0]));
                    return true;
                }
                return false;
            }, "Camera_SetZoom"));

    interpreter.get_global_environment()->define(
        "Camera_GetAngleX", interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                if (ctx && ctx->camera) {
                    std::lock_guard lock(s_CameraMutex);
                    return static_cast<double>(ctx->camera->GetAngleX());
                }
                return 0.0;
            }, "Camera_GetAngleX"));

    interpreter.get_global_environment()->define(
        "Camera_GetAngleZ", interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                if (ctx && ctx->camera) {
                    std::lock_guard lock(s_CameraMutex);
                    return static_cast<double>(ctx->camera->GetAngleZ());
                }
                return 0.0;
            }, "Camera_GetAngleZ"));

    interpreter.get_global_environment()->define(
        "Camera_SetAngle", interpreter.gc.allocate<ObSL::NativeFunction>(
            2,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (ctx && ctx->camera && args.size() >= 2
                    && std::holds_alternative<double>(args[0])
                    && std::holds_alternative<double>(args[1])) {
                    std::lock_guard lock(s_CameraMutex);
                    ctx->camera->SetRotation(
                        static_cast<float>(std::get<double>(args[0])),
                        static_cast<float>(std::get<double>(args[1])));
                    return true;
                }
                return false;
            }, "Camera_SetAngle"));
}
