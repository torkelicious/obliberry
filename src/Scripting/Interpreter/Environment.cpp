#include "Environment.h"
#include <stdexcept>
#include <format>

namespace ObSL {
    void Environment::define(const std::string &name, const Value &value) {
        values[name] = value;
    }

    Value Environment::get(const Token &name) {
        const std::string var_name{name.lexeme};
        if (const auto it = values.find(var_name); it != values.end()) {
            return it->second;
        }
        if (auto parent = enclosing.lock()) {
            return parent->get(name);
        }
        throw std::runtime_error(std::format("Undefined variable '{}'.", var_name));
    }

    void Environment::assign(const Token &name, const Value &value) {
        const std::string var_name{name.lexeme};
        if (const auto it = values.find(var_name); it != values.end()) {
            it->second = value;
            return;
        }

        // safely write to the parent scope
        if (auto parent = enclosing.lock()) {
            parent->assign(name, value);
            return;
        }
        throw std::runtime_error(std::format("Undefined variable '{}'.", var_name));
    }
} // namespace ObSL
