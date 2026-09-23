#include "Scripting/EngineLib/EngineLib.h"
#include "SaveData/SaveGameManager.h"
#include "Scripting/EngineLib/EngineLibFactories.h"
#include <ObSL/Interpreter.h>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace {
    std::optional<Saves::SaveValue> ToSaveValue(const ObSL::Value &value) {
        if (std::holds_alternative<bool>(value)) {
            return std::get<bool>(value);
        }

        if (std::holds_alternative<double>(value)) {
            const double number = std::get<double>(value);
            if (!std::isfinite(number)) {
                return std::nullopt;
            }
            return number;
        }

        if (std::holds_alternative<std::string>(value)) {
            return std::get<std::string>(value);
        }

        return std::nullopt;
    }

    ObSL::Value ToObSLValue(const Saves::SaveValue &value) {
        return std::visit([](const auto &stored) -> ObSL::Value {
            using Type = std::decay_t<decltype(stored)>;

            if constexpr (std::is_same_v<Type, std::int64_t>) {
                return static_cast<double>(stored);
            } else {
                return stored;
            }
        }, value);
    }
} // namespace

void Scripting::EngineLib::register_save_modules(ObSL::Interpreter &interpreter) {
    // set a key value pair in savedata
    interpreter.get_global_environment()->define("save_set", interpreter.gc.allocate<ObSL::NativeFunction>(2, [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (!ctx || !ctx->saveGameManager || args.size() != 2 || !std::holds_alternative<std::string>(args[0])) {
            return false;
        }

        const auto value = ToSaveValue(args[1]);
        if (!value) {
            return false;
        }

        ctx->saveGameManager->Set(std::get<std::string>(args[0]), *value);
        return true;
    }, "save_set"));

    // get value of key
    interpreter.get_global_environment()->define("save_get", interpreter.gc.allocate<ObSL::NativeFunction>(1, [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (!ctx || !ctx->saveGameManager || args.size() != 1 || !std::holds_alternative<std::string>(args[0])) {
            return std::monostate{};
        }

        const auto value = ctx->saveGameManager->Get(std::get<std::string>(args[0]));
        if (!value) {
            return std::monostate{};
        }

        return ToObSLValue(*value);
    }, "save_get"));

    // see if key exists
    interpreter.get_global_environment()->define("save_has", interpreter.gc.allocate<ObSL::NativeFunction>(1, [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (!ctx || !ctx->saveGameManager || args.size() != 1 || !std::holds_alternative<std::string>(args[0])) {
            return false;
        }

        return ctx->saveGameManager->Contains(std::get<std::string>(args[0]));
    }, "save_has"));

    // rm
    interpreter.get_global_environment()->define("save_remove", interpreter.gc.allocate<ObSL::NativeFunction>(1, [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (!ctx || !ctx->saveGameManager || args.size() != 1 || !std::holds_alternative<std::string>(args[0])) {
            return false;
        }

        return ctx->saveGameManager->Remove(std::get<std::string>(args[0]));
    }, "save_remove"));

    // clear pairs
    interpreter.get_global_environment()->define("save_clear", interpreter.gc.allocate<ObSL::NativeFunction>(0, [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
        if (ctx && ctx->saveGameManager) {
            ctx->saveGameManager->ClearValues();
        }
        return std::monostate{};
    }, "save_clear"));

    interpreter.get_global_environment()->define("save_new", interpreter.gc.allocate<ObSL::NativeFunction>(0, [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
        if (ctx && ctx->saveGameManager) {
            ctx->saveGameManager->BeginNewGame();
        }
        return std::monostate{};
    }, "save_new"));

    // disk ops

    interpreter.get_global_environment()->define("save_create", interpreter.gc.allocate<ObSL::NativeFunction>(1, [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (!ctx || !ctx->saveGameManager || args.size() != 1 || !std::holds_alternative<std::string>(args[0])) {
            return std::monostate{};
        }

        const auto filename = ctx->saveGameManager->CreateSave(std::get<std::string>(args[0]));

        if (!filename) {
            return std::monostate{};
        }

        return filename->generic_string();
    }, "save_create"));

    interpreter.get_global_environment()->define("save_write", interpreter.gc.allocate<ObSL::NativeFunction>(0, [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
        if (!ctx || !ctx->saveGameManager) {
            return false;
        }

        return ctx->saveGameManager->SaveActive();
    }, "save_write"));

    interpreter.get_global_environment()->define("save_load", interpreter.gc.allocate<ObSL::NativeFunction>(1, [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (!ctx || !ctx->saveGameManager || args.size() != 1 || !std::holds_alternative<std::string>(args[0])) {
            return false;
        }

        return ctx->saveGameManager->LoadSave(std::get<std::string>(args[0]));
    }, "save_load"));

    interpreter.get_global_environment()->define("save_delete", interpreter.gc.allocate<ObSL::NativeFunction>(1, [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (!ctx || !ctx->saveGameManager || args.size() != 1 || !std::holds_alternative<std::string>(args[0])) {
            return false;
        }

        return ctx->saveGameManager->DeleteSave(std::get<std::string>(args[0]));
    }, "save_delete"));

    interpreter.get_global_environment()->define("save_list", interpreter.gc.allocate<ObSL::NativeFunction>(0, [ctx = m_ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
        auto *result = interp->gc.allocate<ObSL::ObSLArray>();

        EngineLibFactories::GCProtectGuard guard(interp, result);

        if (!ctx || !ctx->saveGameManager) {
            return result;
        }

        for (const auto &save : ctx->saveGameManager->ListSaves()) {
            auto *object = interp->gc.allocate<ObSL::ObSLObject>();
            object->fields["filename"] = save.filename.generic_string();
            object->fields["display_name"] = save.displayName;
            object->fields["created_at"] = static_cast<double>(save.createdAtUtc);
            object->fields["updated_at"] = static_cast<double>(save.updatedAtUtc);
            result->elements.emplace_back(object);
        }

        return result;
    }, "save_list"));
}
