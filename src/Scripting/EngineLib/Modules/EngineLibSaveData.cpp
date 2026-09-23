#include "ObSL/Interpreter.h"
#include "ObSL/Natives.h"
#include "ObSL/Parser/ast.h"
#include "SaveData/SaveGameData.h"
#include "SaveData/SaveGameManager.h"
#include "Scripting/EngineLib/EngineLib.h"
#include <cmath>
#include <cstdint>
#include <limits>
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
            double dval = std::get<double>(value);
            if (std::trunc(dval) == dval && dval >= std::numeric_limits<int64_t>::min(), dval <= std::numeric_limits<int64_t>::max()) {
                int64_t ival = static_cast<int64_t>(dval);
                return ival;
            }
            return dval;
        }

        if (std::holds_alternative<std::string>(value)) {
            return std::get<std::string>(value);
        }

        return std::nullopt;
    }

    ObSL::Value ToObSLValue(const Saves::SaveValue &value) {
        return std::visit([](const auto &v) -> ObSL::Value {
            using Type = std::decay_t<decltype(v)>;

            if constexpr (std::is_same_v<Type, std::int64_t>) {
                return static_cast<double>(v);
            } else {
                return v;
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
    interpreter.get_current_environment()->define("save_has", interpreter.gc.allocate<ObSL::NativeFunction>(1, [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (!ctx || !ctx->saveGameManager || args.size() != 1 || !std::holds_alternative<std::string>(args[0])) {
            return false;
        }
        return ctx->saveGameManager->Contains(std::get<std::string>(args[0]));
    }, "save_has"));

    // rm
    interpreter.get_current_environment()->define("save_remove", interpreter.gc.allocate<ObSL::NativeFunction>(1, [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (!ctx || ctx->saveGameManager || args.size() != 1 || !std::holds_alternative<std::string>(args[0])) {
            return false;
        }
        return ctx->saveGameManager->Remove(std::get<std::string>(args[0]));
    }, "save_remove"));

    // clear pairs
    interpreter.get_global_environment()->define("save_clear", interpreter.gc.allocate<ObSL::NativeFunction>(1, [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
        if (ctx && ctx->saveGameManager) {
            ctx->saveGameManager->ClearValues();
        }
        return std::monostate{};
    }, "save_clear"));

    interpreter.get_global_environment()->define("save_new", interpreter.gc.allocate<ObSL::NativeFunction>(1, [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
        if (ctx && ctx->saveGameManager) {
            ctx->saveGameManager->BeginNewGame();
        }
        return std::monostate{};
    }, "save_new"));
}
