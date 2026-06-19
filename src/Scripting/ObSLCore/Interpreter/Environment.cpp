#include "Environment.h"
#include <stdexcept>
#include <format>

namespace ObSL {
    void Environment::define(const std::string_view name, const Value &value) {
        values[std::string(name)] = value;
    }

    Value Environment::get(const Token &name) {
        if (const auto it = values.find(name.lexeme); it != values.end()) {
            return it->second;
        }
        if (enclosing) {
            return enclosing->get(name);
        }
        throw std::runtime_error(std::format("Undefined variable '{}'.", name.lexeme));
    }

    Value Environment::get(const std::string_view name) {
        if (const auto it = values.find(name); it != values.end()) {
            return it->second;
        }
        if (enclosing) {
            return enclosing->get(name);
        }
        throw std::runtime_error(std::format("Undefined variable '{}'.", name));
    }

    void Environment::assign(const Token &name, const Value &value) {
        if (const auto it = values.find(name.lexeme); it != values.end()) {
            it->second = value;
            return;
        }

        if (enclosing) {
            enclosing->assign(name, value);
            return;
        }
        throw std::runtime_error(std::format("Undefined variable '{}'.", name.lexeme));
    }

    void Environment::assign(const std::string_view name, const Value &value) {
        if (const auto it = values.find(name); it != values.end()) {
            it->second = value;
            return;
        }

        if (enclosing) {
            enclosing->assign(name, value);
            return;
        }
        throw std::runtime_error(std::format("Undefined variable '{}'.", name));
    }
} // namespace ObSL
