#pragma once

#include "Core/UUID.h"
#include "ObSL/Parser/ast.h"
#include <mutex>
#include <string>
#include <utility>
#include <variant>
#include <vector>
namespace Scripting::ScriptMessages {

    struct Message {
        bool broadcast = false;
        Core::UUID target{}; // unused for broadcasts
        std::string name;
        ObSL::Value value;
    };

    inline std::mutex g_Mutex;
    inline std::vector<Message> g_Pending;

    // may cross interps
    inline bool IsPrimitive(const ObSL::Value &v) { return std::holds_alternative<std::monostate>(v) || std::holds_alternative<bool>(v) || std::holds_alternative<double>(v) || std::holds_alternative<std::string>(v); }

    inline void Push(Message msg) {
        std::lock_guard lock(g_Mutex);
        g_Pending.push_back(std::move(msg));
    }

    inline std::vector<Message> Take() {
        std::lock_guard lock(g_Mutex);
        return std::exchange(g_Pending, {});
    }

    inline void Clear() {
        std::lock_guard lock(g_Mutex);
        g_Pending.clear();
    }

} // namespace Scripting::ScriptMessages
