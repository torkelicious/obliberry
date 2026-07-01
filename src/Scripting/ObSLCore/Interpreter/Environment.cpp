#include "Environment.h"
#include <stdexcept>
#include <format>
#include <mutex>

namespace ObSL {
    void Environment::define(const std::string_view name, const Value &value) {
        std::unique_lock lock(env_mutex);
        values[std::string(name)] = value;
    }

    Value Environment::get(const Token &name) {
        std::shared_lock lock(env_mutex);
        if (const auto it = values.find(name.lexeme); it != values.end()) {
            return it->second;
        }
        if (enclosing) {
            // Release lock before recursive call to avoid deadlocks
            lock.unlock();
            return enclosing->get(name);
        }
        throw std::runtime_error(std::format("Undefined variable '{}'.", name.lexeme));
    }

    Value Environment::get(const std::string_view name) {
        std::shared_lock lock(env_mutex);
        if (const auto it = values.find(name); it != values.end()) {
            return it->second;
        }
        if (enclosing) {
            // Release lock before recursive call to avoid deadlocks
            lock.unlock();
            return enclosing->get(name);
        }
        throw std::runtime_error(std::format("Undefined variable '{}'.", name));
    }

    void Environment::assign(const Token &name, const Value &value) {
        std::unique_lock lock(env_mutex);
        if (const auto it = values.find(name.lexeme); it != values.end()) {
            it->second = value;
            return;
        }

        if (enclosing) {
            // Release lock before recursive call to avoid deadlocks
            lock.unlock();
            enclosing->assign(name, value);
            return;
        }
        throw std::runtime_error(std::format("Undefined variable '{}'.", name.lexeme));
    }

    void Environment::assign(const std::string_view name, const Value &value) {
        std::unique_lock lock(env_mutex);
        if (const auto it = values.find(name); it != values.end()) {
            it->second = value;
            return;
        }

        if (enclosing) {
            // Release lock before recursive call to avoid deadlocks
            lock.unlock();
            enclosing->assign(name, value);
            return;
        }
        throw std::runtime_error(std::format("Undefined variable '{}'.", name));
    }
} // namespace ObSL
