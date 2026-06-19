#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include <string_view>
#include "Scripting/ObSLCore/Parser/ast.h"

namespace ObSL {
    struct StringHash {
        using is_transparent = void;

        [[nodiscard]] size_t operator()(const std::string_view txt) const {
            return std::hash<std::string_view>{}(txt);
        }
    };

    class Environment {
    private:
        std::unordered_map<std::string, Value, StringHash, std::equal_to<> > values;
        std::shared_ptr<Environment> enclosing;

    public:
        Environment() = default;

        explicit Environment(const std::shared_ptr<Environment> &enclosing)
            : enclosing(enclosing) {
        }

        void clear() {
            values.clear();
        }

        const std::unordered_map<std::string, Value, StringHash, std::equal_to<> > &get_values() const {
            return values;
        }

        void define(std::string_view name, const Value &value);

        Value get(const Token &name);

        Value get(std::string_view name);

        void assign(const Token &name, const Value &value);

        void assign(std::string_view name, const Value &value);

        void mark() {
            for (auto &val: values | std::views::values) {
                mark_value(val);
            }
            if (enclosing) {
                enclosing->mark();
            }
        }
    };
} // namespace ObSL
