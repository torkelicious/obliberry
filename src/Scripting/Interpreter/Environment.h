#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include "Scripting/Parser/ast.h"

namespace ObSL {
    class Environment {
    private:
        std::unordered_map<std::string, Value> values;
        std::shared_ptr<Environment> enclosing;

    public:
        Environment() : enclosing(nullptr) {
        }

        explicit Environment(std::shared_ptr<Environment> enclosing)
            : enclosing(std::move(enclosing)) {
        }

        const std::unordered_map<std::string, Value> &get_values() const { return values; }

        void define(const std::string &name, const Value &value);

        Value get(const Token &name);

        void assign(const Token &name, const Value &value);
    };
} // namespace ObSL
