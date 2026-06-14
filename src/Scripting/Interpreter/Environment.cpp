#include "Environment.h"

namespace ObSL {
    void Environment::define(const std::string &name, const Value &value) {
        values[name] = value;
    }

    Value Environment::get(const Token &name) {
        std::string var_name = std::string(name.lexeme);
        //  check in current scope
        if (auto it = values.find(var_name); it != values.end()) {
            return it->second;
        }
        // check in enclosing scope
        if (enclosing != nullptr) {
            return enclosing->get(name);
        }
        throw std::runtime_error(std::format("Undefined variable '{}'.", var_name));
    }

    void Environment::assign(const Token &name, const Value &value) {
        std::string var_name = std::string(name.lexeme);
        // check current scope
        if (auto it = values.find(var_name); it != values.end()) {
            it->second = value;
            return;
        }

        // check enclosing scope
        if (enclosing != nullptr) {
            enclosing->assign(name, value);
            return;
        }
        throw std::runtime_error(std::format("Undefined variable '{}'.", var_name));
    }
} // ObSL
