#include "Interpreter.h"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <format>
#include <fstream>
#include <type_traits>
#include <sstream>
#include <memory_resource>
#include <ranges>
#include <unordered_set>

#include "Scripting/ObSLCore/Parser/Parser.h"

using namespace std::string_view_literals;

namespace ObSL {
    struct EnvironmentGuard {
        std::shared_ptr<Environment> &m_env_ref;
        std::shared_ptr<Environment> m_previous;

        EnvironmentGuard(std::shared_ptr<Environment> &env, std::shared_ptr<Environment> new_env)
            : m_env_ref(env), m_previous(env) {
            m_env_ref = std::move(new_env);
        }

        ~EnvironmentGuard() {
            m_env_ref = std::move(m_previous);
        }

        EnvironmentGuard(const EnvironmentGuard &) = delete;

        EnvironmentGuard &operator=(const EnvironmentGuard &) = delete;
    };

    void Interpreter::interpret(const std::vector<std::unique_ptr<Stmt> > &statements) {
        try {
            for (const auto &stmt: statements) {
                if (stmt) execute(stmt.get());
            }
        } catch (const RuntimeError &error) {
            std::cerr << error.what() << "\n";
        }
    }

    void Interpreter::execute(const Stmt *stmt) {
        switch (stmt->type()) {
            case StmtType::Using: return execute_using_stmt(static_cast<const UsingStmt *>(stmt));
            case StmtType::Expression: return execute_expression_stmt(static_cast<const ExpressionStmt *>(stmt));
            case StmtType::Print: return execute_print_stmt(static_cast<const PrintStmt *>(stmt));
            case StmtType::Println: return execute_print_ln_stmt(static_cast<const PrintlnStmt *>(stmt));
            case StmtType::Var: return execute_var_stmt(static_cast<const VarStmt *>(stmt));
            case StmtType::Block: return execute_block_stmt(static_cast<const BlockStmt *>(stmt));
            case StmtType::If: return execute_if_stmt(static_cast<const IfStmt *>(stmt));
            case StmtType::Switch: return execute_switch_stmt(static_cast<const SwitchStmt *>(stmt));
            case StmtType::While: return execute_while_stmt(static_cast<const WhileStmt *>(stmt));
            case StmtType::Foreach: return execute_foreach_stmt(static_cast<const ForeachStmt *>(stmt));
            case StmtType::Break: return execute_break_stmt(static_cast<const BreakStmt *>(stmt));
            case StmtType::Return: return execute_return_stmt(static_cast<const ReturnStmt *>(stmt));
            case StmtType::Function: return execute_function_stmt(static_cast<const FunctionStmt *>(stmt));
            case StmtType::TryCatch: return execute_try_catch_stmt(static_cast<const TryCatchStmt *>(stmt));
            case StmtType::Struct: return execute_struct_stmt(static_cast<const StructStmt *>(stmt));
        }
        throw std::runtime_error("Unknown statement type in interpreter.");
    }

    Value Interpreter::evaluate(const Expr *expr) {
        switch (expr->type()) {
            case ExprType::Literal: return evaluate_literal(static_cast<const LiteralExpr *>(expr));
            case ExprType::Variable: return evaluate_variable(static_cast<const VariableExpr *>(expr));
            case ExprType::Binary: return evaluate_binary(static_cast<const BinaryExpr *>(expr));
            case ExprType::Grouping: return evaluate_grouping(static_cast<const GroupingExpr *>(expr));
            case ExprType::Unary: return evaluate_unary(static_cast<const UnaryExpr *>(expr));
            case ExprType::Update: return evaluate_update(static_cast<const UpdateExpr *>(expr));
            case ExprType::Assignment: return evaluate_assignment(static_cast<const AssignmentExpr *>(expr));
            case ExprType::Array: return evaluate_array(static_cast<const ArrayExpr *>(expr));
            case ExprType::Index: return evaluate_index(static_cast<const IndexExpr *>(expr));
            case ExprType::IndexAssignment: return evaluate_index_assignment(
                    static_cast<const IndexAssignmentExpr *>(expr));
            case ExprType::Logical: return evaluate_logical(static_cast<const LogicalExpr *>(expr));
            case ExprType::TypeCheck: return evaluate_type_check(static_cast<const TypeCheckExpr *>(expr));
            case ExprType::Call: return evaluate_call(static_cast<const CallExpr *>(expr));
            case ExprType::Get: return evaluate_get(static_cast<const GetExpr *>(expr));
            case ExprType::Set: return evaluate_set(static_cast<const SetExpr *>(expr));
        }
        throw std::runtime_error("Unknown expression type in interpreter.");
    }

    void Interpreter::execute_function_stmt(const FunctionStmt *stmt) {
        auto *function = gc.allocate<ObSLFunction>(stmt, environment);
        environment->define(stmt->name, function);
    }

    Value Interpreter::evaluate_call(const CallExpr *expr) {
        const GCProtectScope scope(this);

        const Value callee = evaluate(expr->callee.get());
        scope.protect(callee);

        std::vector<Value> arguments;
        arguments.reserve(expr->arguments.size());
        for (const auto &arg: expr->arguments) {
            Value val = evaluate(arg.get());
            scope.protect(val);
            arguments.push_back(val);
        }

        if (!std::holds_alternative<ObSLCallable *>(callee)) {
            throw RuntimeError(expr->paren, "Can only call functions.");
        }

        const auto function = std::get<ObSLCallable *>(callee);
        if (arguments.size() < static_cast<size_t>(function->min_arity()) || arguments.size() > static_cast<size_t>(
                function->arity())) {
            throw RuntimeError(expr->paren, std::format("Expected between {} and {} arguments but got {}.",
                                                        function->min_arity(), function->arity(), arguments.size()));
        }
        return function->call(this, arguments, expr->paren);
    }

    void Interpreter::execute_expression_stmt(const ExpressionStmt *stmt) {
        static_cast<void>(evaluate(stmt->expression.get()));
    }

    void Interpreter::execute_print_stmt(const PrintStmt *stmt) {
        Value value = evaluate(stmt->expression.get());
        std::visit([this]<typename T0>(const T0 &arg) {
            using T = std::decay_t<T0>;
            if constexpr (std::is_same_v<T, std::monostate>) m_stdout << "null";
            else if constexpr (std::is_same_v<T, bool>) m_stdout << (arg ? "true" : "false");
            else if constexpr (std::is_same_v<T, double>) m_stdout << arg;
            else if constexpr (std::is_same_v<T, std::string>) m_stdout << arg;
            else if constexpr (std::is_same_v<T, ObSLCallable *>) m_stdout << (arg ? arg->to_string() : "null");
            else if constexpr (std::is_same_v<T, ObSLArray *>) m_stdout << (arg ? "[Array]" : "null");
            else if constexpr (std::is_same_v<T, ObSLObject *>) m_stdout << (arg ? "[Object]" : "null");
        }, value);
    }

    void Interpreter::execute_print_ln_stmt(const PrintlnStmt *stmt) {
        execute_print_stmt(reinterpret_cast<const PrintStmt *>(stmt));
        m_stdout << "\n";
    }

    void Interpreter::execute_var_stmt(const VarStmt *stmt) {
        Value value = std::monostate{};
        if (stmt->initializer) value = evaluate(stmt->initializer.get());
        environment->define(stmt->name, value);
    }

    Value Interpreter::evaluate_literal(const LiteralExpr *expr) {
        return expr->value;
    }

    Value Interpreter::evaluate_variable(const VariableExpr *expr) const {
        return environment->get(expr->name);
    }

    Value Interpreter::evaluate_array(const ArrayExpr *expr) {
        const GCProtectScope scope(this);
        auto *array = gc.allocate<ObSLArray>();
        scope.protect(array);

        array->elements.reserve(expr->elements.size());
        for (const auto &item: expr->elements) {
            Value val = evaluate(item.get());
            scope.protect(val);
            array->elements.push_back(val);
        }
        return array;
    }

    Value Interpreter::evaluate_index(const IndexExpr *expr) {
        const GCProtectScope scope(this);
        const Value callee = evaluate(expr->callee.get());
        scope.protect(callee);
        const Value index_val = evaluate(expr->index.get());

        if (std::holds_alternative<ObSLArray *>(callee)) {
            const auto array = std::get<ObSLArray *>(callee);
            if (!std::holds_alternative<double>(index_val))
                throw RuntimeError(
                    expr->bracket, "Array index must be a number.");
            const int index = static_cast<int>(std::get<double>(index_val));
            if (index < 0 || index >= array->elements.size())
                throw RuntimeError(
                    expr->bracket, "Array index out of bounds.");
            return array->elements[index];
        }
        throw RuntimeError(expr->bracket, "Only arrays can be indexed.");
    }

    Value Interpreter::evaluate_index_assignment(const IndexAssignmentExpr *expr) {
        GCProtectScope scope(this);
        Value callee = evaluate(expr->callee.get());
        scope.protect(callee);
        Value index_val = evaluate(expr->index.get());
        scope.protect(index_val);
        Value value = evaluate(expr->value.get());

        if (std::holds_alternative<ObSLArray *>(callee)) {
            auto array = std::get<ObSLArray *>(callee);

            if (!std::holds_alternative<double>(index_val)) {
                throw RuntimeError(expr->bracket, "Array index must be a number.");
            }
            int index = static_cast<int>(std::get<double>(index_val));
            int array_size = static_cast<int>(array->elements.size());

            // negative indexing
            if (index < 0) {
                index += array_size;
            }
            if (index < 0 || index >= array_size) {
                throw RuntimeError(expr->bracket, "Array index out of bounds.");
            }
            array->elements[index] = value;
            return value;
        }
        throw RuntimeError(expr->bracket, "Only collections can be indexed for assignment.");
    }

    Value Interpreter::evaluate_binary(const BinaryExpr *expr) {
        const GCProtectScope scope(this);
        const Value lhs = evaluate(expr->left.get());
        scope.protect(lhs);
        const Value rhs = evaluate(expr->right.get());

        switch (expr->oprt_type) {
            case TokenType::GREATER:
                check_number_operands(expr->oprt_type, lhs, rhs);
                return std::get<double>(lhs) > std::get<double>(rhs);
            case TokenType::GREATER_EQUAL:
                check_number_operands(expr->oprt_type, lhs, rhs);
                return std::get<double>(lhs) >= std::get<double>(rhs);
            case TokenType::LESS:
                check_number_operands(expr->oprt_type, lhs, rhs);
                return std::get<double>(lhs) < std::get<double>(rhs);
            case TokenType::LESS_EQUAL:
                check_number_operands(expr->oprt_type, lhs, rhs);
                return std::get<double>(lhs) <= std::get<double>(rhs);
            case TokenType::BANG_EQUAL:
                return !is_equal(lhs, rhs);
            case TokenType::EQUAL_EQUAL:
                return is_equal(lhs, rhs);
            case TokenType::MINUS:
                check_number_operands(expr->oprt_type, lhs, rhs);
                return std::get<double>(lhs) - std::get<double>(rhs);
            case TokenType::PLUS:
                if (std::holds_alternative<std::string>(lhs) || std::holds_alternative<std::string>(rhs)) {
                    auto stringify = [](const Value &val) -> std::string {
                        if (std::holds_alternative<std::string>(val)) return std::get<std::string>(val);
                        if (std::holds_alternative<double>(val)) return std::format("{:g}", std::get<double>(val));
                        if (std::holds_alternative<bool>(val)) return std::get<bool>(val) ? "true" : "false";
                        return "null";
                    };
                    return stringify(lhs) + stringify(rhs);
                }
                if (std::holds_alternative<double>(lhs) && std::holds_alternative<double>(rhs)) {
                    return std::get<double>(lhs) + std::get<double>(rhs);
                }
                throw RuntimeError(Token{TokenType::UNKNOWN, "binary", 0, 0, 0, 0},
                                   "Operands must be numbers or strings.");
            case TokenType::SLASH:
                check_number_operands(expr->oprt_type, lhs, rhs);
                if (std::get<double>(rhs) == 0)
                    throw RuntimeError(Token{expr->oprt_type, "", 0, 0, 0, 0},
                                       "Division by zero.");
                return std::get<double>(lhs) / std::get<double>(rhs);
            case TokenType::PERCENT:
                check_number_operands(expr->oprt_type, lhs, rhs);
                if (std::get<double>(rhs) == 0)
                    throw RuntimeError(Token{expr->oprt_type, "", 0, 0, 0, 0},
                                       "Modulo by zero.");
                return std::fmod(std::get<double>(lhs), std::get<double>(rhs));
            case TokenType::STAR:
                check_number_operands(expr->oprt_type, lhs, rhs);
                return std::get<double>(lhs) * std::get<double>(rhs);
            case TokenType::AMPERSAND:
                check_number_operands(expr->oprt_type, lhs, rhs);
                return static_cast<double>(static_cast<int64_t>(std::get<double>(lhs)) & static_cast<int64_t>(std::get<
                                               double>(rhs)));
            case TokenType::PIPE:
                check_number_operands(expr->oprt_type, lhs, rhs);
                return static_cast<double>(static_cast<int64_t>(std::get<double>(lhs)) | static_cast<int64_t>(std::get<
                                               double>(rhs)));
            case TokenType::CARET:
                check_number_operands(expr->oprt_type, lhs, rhs);
                return static_cast<double>(static_cast<int64_t>(std::get<double>(lhs)) ^ static_cast<int64_t>(std::get<
                                               double>(rhs)));
            case TokenType::LESS_LESS:
                check_number_operands(expr->oprt_type, lhs, rhs);
                return static_cast<double>(static_cast<int64_t>(std::get<double>(lhs)) << static_cast<int64_t>(std::get<
                                               double>(rhs)));
            case TokenType::GREATER_GREATER:
                check_number_operands(expr->oprt_type, lhs, rhs);
                return static_cast<double>(static_cast<int64_t>(std::get<double>(lhs)) >> static_cast<int64_t>(std::get<
                                               double>(rhs)));
            default: break;
        }
        return std::monostate{};
    }

    Value Interpreter::evaluate_grouping(const GroupingExpr *expr) { return evaluate(expr->expr.get()); }

    Value Interpreter::evaluate_unary(const UnaryExpr *expr) {
        const Value right = evaluate(expr->right.get());
        switch (expr->oprt_type) {
            case TokenType::MINUS:
                check_number_operand(expr->oprt_type, right);
                return -std::get<double>(right);
            case TokenType::BANG:
                return !is_truthy(right);
            case TokenType::TILDE:
                if (std::holds_alternative<double>(right)) {
                    return static_cast<double>(~static_cast<int64_t>(std::get<double>(right)));
                }
                throw RuntimeError(Token{TokenType::UNKNOWN, "unary", 0, 0, 0, 0}, "Operand must be a number.");
            default: break;
        }
        return std::monostate{};
    }

    Value Interpreter::evaluate_update(const UpdateExpr *expr) const {
        const Value current_value = environment->get(expr->name);
        check_number_operand(expr->oprt_type, current_value);
        double num = std::get<double>(current_value);
        double new_num = expr->oprt_type == TokenType::PLUS_PLUS ? num + 1.0 : num - 1.0;
        environment->assign(expr->name, Value(new_num));
        return expr->is_prefix ? Value(new_num) : Value(num);
    }

    Value Interpreter::evaluate_assignment(const AssignmentExpr *expr) {
        Value value = evaluate(expr->value.get());
        environment->assign(expr->name, value);
        return value;
    }

    Value Interpreter::evaluate_logical(const LogicalExpr *expr) {
        Value left = evaluate(expr->left.get());
        if (expr->oprt_type == TokenType::OR) {
            if (is_truthy(left)) return left;
        } else {
            if (!is_truthy(left)) return left;
        }
        return evaluate(expr->right.get());
    }

    Value Interpreter::evaluate_type_check(const TypeCheckExpr *expr) {
        const Value value = evaluate(expr->left.get());

        if (expr->type_name == "number") {
            return std::holds_alternative<double>(value);
        }
        if (expr->type_name == "string") {
            return std::holds_alternative<std::string>(value);
        }
        if (expr->type_name == "boolean" || expr->type_name == "bool") {
            return std::holds_alternative<bool>(value);
        }
        if (expr->type_name == "null" || expr->type_name == "nil") {
            return std::holds_alternative<std::monostate>(value);
        }
        if (expr->type_name == "function" || expr->type_name == "fn") {
            return std::holds_alternative<ObSLCallable *>(value);
        }
        if (expr->type_name == "array") {
            return std::holds_alternative<ObSLArray *>(value);
        }
        if (expr->type_name == "object") {
            return std::holds_alternative<ObSLObject *>(value);
        }

        throw RuntimeError(Token{TokenType::IS, "is", 0, 0, 0, 0},
                           std::format("Unknown type name '{}'.", expr->type_name));
    }

    bool Interpreter::is_truthy(const Value &value) {
        if (std::holds_alternative<std::monostate>(value)) return false;
        if (std::holds_alternative<bool>(value)) return std::get<bool>(value);
        return true;
    }

    void Interpreter::check_number_operand(const TokenType oprt, const Value &oprnd) {
        if (std::holds_alternative<double>(oprnd)) return;
        throw RuntimeError(Token{oprt, "", 0, 0, 0, 0}, "Operand must be a number.");
    }

    void Interpreter::check_number_operands(const TokenType oprt, const Value &lhs, const Value &rhs) {
        if (!std::holds_alternative<double>(lhs) || !std::holds_alternative<double>(rhs)) {
            throw RuntimeError(Token{oprt, "", 0, 0, 0, 0}, "Operands must be numbers.");
        }
    }

    bool Interpreter::is_equal(const Value &a, const Value &b) {
        return std::visit([](auto &&arg1, auto &&arg2) {
            using T1 = std::decay_t<decltype(arg1)>;
            using T2 = std::decay_t<decltype(arg2)>;
            if constexpr (std::is_same_v<T1, T2>) return arg1 == arg2;
            else return false;
        }, a, b);
    }

    void Interpreter::execute_using_stmt(const UsingStmt *stmt) {
        std::string module_name = std::filesystem::path(stmt->path).stem().string();
        if (loaded_modules.contains(stmt->path)) {
            environment->define(module_name, loaded_modules[stmt->path]);
            return;
        }
        std::ifstream file(stmt->path);
        if (!file.is_open())
            throw RuntimeError(Token{TokenType::UNKNOWN, "using", 0, 0, 0, 0},
                               std::format("Could not open module '{}'.", stmt->path));

        std::stringstream buffer;
        buffer << file.rdbuf();

        module_sources.push_back(buffer.str());
        Lexer lexer(module_sources.back());
        auto tokens = lexer.tokenize();

        Parser parser(tokens);

        module_asts.push_back(parser.parse());
        const auto &statements = module_asts.back();

        auto *module_obj = gc.allocate<ObSLObject>();
        GCProtectScope scope(this);
        scope.protect(module_obj);

        auto module_env = std::make_shared<Environment>(globals);
        register_environment(module_env);
        auto previous_env = environment;

        try {
            environment = module_env;
            for (const auto &module_stmt: statements | std::views::filter([](auto &s) { return s != nullptr; })) {
                execute(module_stmt.get());
            }

            for (const auto &[name, val]: module_env->get_values()) {
                module_obj->fields[name] = val;
            }
            environment = previous_env;
        } catch (...) {
            environment = previous_env;
            throw;
        }

        loaded_modules[stmt->path] = module_obj;
        environment->define(module_name, module_obj);

        if (loaded_modules.size() > max_loaded_modules) {
            size_t to_remove = loaded_modules.size() / 2;
            for (auto it = loaded_modules.begin(); to_remove > 0 && it != loaded_modules.end(); ++it) {
                it = loaded_modules.erase(it);
                --to_remove;
            }
        }
    }

    void Interpreter::execute_try_catch_stmt(const TryCatchStmt *stmt) {
        try {
            execute_block_stmt(stmt->try_body.get());
        } catch (const RuntimeError &error) {
            const auto catch_env = std::make_shared<Environment>(environment);
            register_environment(catch_env);
            catch_env->define(stmt->exception_var, std::string(error.what()));
            execute_block(stmt->catch_body->statements, catch_env);
        }
    }

    void Interpreter::execute_struct_stmt(const StructStmt *stmt) {
        auto *struct_def = gc.allocate<ObSLStruct>(stmt);
        environment->define(stmt->name.lexeme, struct_def);
    }


    void Interpreter::execute_block(const std::span<const std::unique_ptr<Stmt>> statements,
                                    std::shared_ptr<Environment> block_env) {
        EnvironmentGuard guard(environment, environment);
        environment = std::move(block_env);
        for (const auto &stmt: statements) {
            if (stmt) execute(stmt.get());
        }
    }

    void Interpreter::execute_block_stmt(const BlockStmt *stmt) {
        const auto block_env = std::make_shared<Environment>(environment);
        register_environment(block_env);
        EnvironmentGuard guard(environment, environment);
        execute_block(std::span(stmt->statements), block_env);
    }

    void Interpreter::execute_if_stmt(const IfStmt *stmt) {
        if (is_truthy(evaluate(stmt->condition.get()))) execute(stmt->then_branch.get());
        else if (stmt->else_branch) execute(stmt->else_branch.get());
    }

    void Interpreter::execute_switch_stmt(const SwitchStmt *stmt) {
        const Value condition_val = evaluate(stmt->condition.get());
        try {
            const CaseBranch *default_branch = nullptr;
            for (const auto &case_branch: stmt->cases) {
                if (case_branch.match_value == nullptr) {
                    default_branch = &case_branch;
                    continue;
                }
                if (Value case_val = evaluate(case_branch.match_value.get()); is_equal(condition_val, case_val)) {
                    for (const auto &case_stmt: case_branch.statements | std::views::filter([](auto &s) {
                        return s != nullptr;
                    })) {
                        execute(case_stmt.get());
                    }
                    return;
                }
            }
            if (default_branch != nullptr) {
                for (const auto &case_stmt: default_branch->statements | std::views::filter([](auto &s) {
                    return s != nullptr;
                })) {
                    execute(case_stmt.get());
                }
            }
        } catch (const BreakException &) {
        }
    }


    void Interpreter::execute_while_stmt(const WhileStmt *stmt) {
        while (is_truthy(evaluate(stmt->condition.get()))) {
            try { execute(stmt->body.get()); } catch (const BreakException &) { break; }
        }
    }

    void Interpreter::execute_foreach_stmt(const ForeachStmt *stmt) {
        if (const Value iterable_val = evaluate(stmt->iterable.get()); std::holds_alternative<ObSLArray
            *>(iterable_val)) {
            for (const auto array = std::get<ObSLArray *>(iterable_val); const auto &item: array->elements) {
                auto loop_env = std::make_shared<Environment>(environment);
                register_environment(loop_env);
                loop_env->define(stmt->loop_var, item);
                EnvironmentGuard guard(environment, environment);
                environment = std::move(loop_env);
                try { execute(stmt->body.get()); } catch (const BreakException &) { break; }
            }
        } else {
            throw RuntimeError(Token{TokenType::UNKNOWN, stmt->loop_var, 0, 0, 0, 0},
                               "Object is not iterable. Expected an Array.");
        }
    }

    void Interpreter::execute_break_stmt(const BreakStmt *) { throw BreakException(); }

    void Interpreter::execute_return_stmt(const ReturnStmt *stmt) {
        Value value = std::monostate{};
        if (stmt->value) value = evaluate(stmt->value.get());
        throw ReturnException(value);
    }

    Value Interpreter::evaluate_get(const GetExpr *expr) {
        const Value obj = evaluate(expr->obj.get());

        if (std::holds_alternative<ObSLArray *>(obj)) {
            const auto array = std::get<ObSLArray *>(obj);
            if (expr->name == "len") return static_cast<double>(array->elements.size());
            if (expr->name == "push") {
                auto push_fn = [array](Interpreter *, const std::vector<Value> &args)-> Value {
                    array->elements.push_back(args[0]);
                    return args[0];
                };
                return gc.allocate<NativeFunction>(1, std::move(push_fn), "push");
            }
            if (expr->name == "pop") {
                auto pop_fn = [array](Interpreter *, const std::vector<Value> &)-> Value {
                    if (array->elements.empty()) return std::monostate{}; // return null if empty
                    Value val = array->elements.back();
                    array->elements.pop_back();
                    return val;
                };
                return gc.allocate<NativeFunction>(0, std::move(pop_fn), "pop");
            }
            if (expr->name == "clear") {
                auto clear_fn = [array](Interpreter *, const std::vector<Value> &)-> Value {
                    array->elements.clear();
                    return std::monostate{};
                };
                return gc.allocate<NativeFunction>(0, std::move(clear_fn), "clear");
            }

            throw RuntimeError(Token{TokenType::IDENTIFIER, expr->name, 0, 0, 0, 0},
                               std::format("Undefined property '{}' on Array.", expr->name));
        }

        if (std::holds_alternative<ObSLObject *>(obj)) {
            const auto instance = std::get<ObSLObject *>(obj);
            const std::string prop_name(expr->name);

            if (const auto it = instance->fields.find(prop_name); it != instance->fields.end()) {
                Value val = it->second;
                if (std::holds_alternative<ObSLCallable *>(val)) {
                    const auto callable = std::get<ObSLCallable *>(val);
                    if (const auto func = dynamic_cast<ObSLFunction *>(callable)) {
                        return func->bind(instance, this);
                    }
                }
                return val;
            }
            throw RuntimeError(Token{TokenType::IDENTIFIER, expr->name, 0, 0, 0, 0},
                               std::format("Undefined property '{}' on object.", expr->name));
        }
        throw RuntimeError(Token{TokenType::IDENTIFIER, expr->name, 0, 0, 0, 0}, " does not contain property.");
    }

    Value Interpreter::evaluate_set(const SetExpr *expr) {
        const GCProtectScope scope(this);
        const Value obj = evaluate(expr->obj.get());
        scope.protect(obj);

        if (!std::holds_alternative<ObSLObject *>(obj)) {
            throw RuntimeError(Token{TokenType::IDENTIFIER, expr->name, 0, 0, 0, 0},
                               "Only objects can have fields assigned.");
        }
        const auto instance = std::get<ObSLObject *>(obj);
        Value value = evaluate(expr->value.get());
        instance->fields[std::string(expr->name)] = value;
        return value;
    }


    int ObSLFunction::arity() const { return static_cast<int>(declaration->params.size()); }

    int ObSLFunction::min_arity() const {
        int min_args = 0;
        for (const auto &[name, default_value]: declaration->params) {
            if (default_value == nullptr) min_args++;
        }
        return min_args;
    }

    Value ObSLFunction::call(Interpreter *interpreter, const std::vector<Value> &arguments, const Token &call_token) {
        const size_t max_arity = declaration->params.size();
        const auto environment = std::make_shared<Environment>(closure);
        interpreter->register_environment(environment);
        const auto previous_env = interpreter->get_current_environment();

        try {
            for (size_t i = 0; i < max_arity; ++i) {
                const auto &[name, default_value] = declaration->params[i];
                Value final_val;

                if (i < arguments.size()) {
                    final_val = arguments[i];
                } else {
                    interpreter->set_current_environment(environment);
                    final_val = interpreter->evaluate(default_value.get());
                }
                environment->define(name, final_val);
            }
            interpreter->set_current_environment(environment);
            interpreter->execute_block(declaration->body->statements, environment);
        } catch (const ReturnException &returnValue) {
            interpreter->set_current_environment(previous_env);
            return returnValue.value;
        } catch (...) {
            interpreter->set_current_environment(previous_env);
            throw;
        }
        interpreter->set_current_environment(previous_env);
        return std::monostate{};
    }

    std::string ObSLFunction::to_string() const {
        return std::format("<fn {}>", declaration->name);
    }

    Value ObSLStruct::call(Interpreter *interpreter, const std::vector<Value> &arguments, const Token &call_token) {
        auto *instance = interpreter->gc.allocate<ObSLObject>();

        for (size_t i = 0; i < declaration->fields.size(); ++i) {
            const auto &[name, default_value] = declaration->fields[i];
            auto field_name = std::string(name.lexeme);

            if (i < arguments.size()) {
                instance->fields[field_name] = arguments[i];
            } else if (default_value) {
                instance->fields[field_name] = interpreter->evaluate(default_value.get());
            } else {
                instance->fields[field_name] = std::monostate{};
            }
        }

        return instance;
    }
} // namespace ObSL
