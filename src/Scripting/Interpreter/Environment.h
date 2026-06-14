#ifndef OBLIBERRY_ENVIRONMENT_H
#define OBLIBERRY_ENVIRONMENT_H
#include <memory>
#include <unordered_map>

#include "Scripting/Parser/ast.h"

namespace ObSL {
    class Environment {
    private:
        // symbol table for this scope
        std::unordered_map<std::string, Value> values;

        // pointer to scope above it
        std::shared_ptr<Environment> enclosing;

    public:
        // global scope
        Environment() : enclosing(nullptr) {
        }

        // local scope
        explicit Environment(std::shared_ptr<Environment> enclosing) : enclosing(std::move(enclosing)) {
        }

        // create new var in current scope
        void define(const std::string &name, const Value &value);

        // fetch var
        Value get(const Token &name);

        // update var
        void assign(const Token &name, const Value &value);
    };
} // ObSL

#endif //OBLIBERRY_ENVIRONMENT_H
