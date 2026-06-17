#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include <string_view>
#include "Scripting/Parser/ast.h"

namespace ObSL {
    class Environment {
    private:
        std::unordered_map<std::string, Value> values;
        std::shared_ptr<Environment> enclosing;

    public:
        Environment() = default;

        explicit Environment(const std::shared_ptr<Environment> &enclosing)
            : enclosing(enclosing) {
        }

        void clear() {
            values.clear();
        }

        const std::unordered_map<std::string, Value> &get_values() const { return values; }

        void define(std::string_view name, const Value &value);

        Value get(const Token &name);

        Value get(std::string_view name);

        void assign(const Token &name, const Value &value);

        void assign(std::string_view name, const Value &value);
    };
} // namespace ObSL
