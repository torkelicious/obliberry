#include "Interpreter.h"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <format>
#include <fstream>
#include <type_traits>
#include <sstream>
#include "Scripting/Parser/Parser.h"

namespace ObSL {
    struct EnvironmentGuard {
        std::shared_ptr<Environment> &current_env;
        std::shared_ptr<Environment> previous_env;

        EnvironmentGuard(std::shared_ptr<Environment> &env, std::shared_ptr<Environment> prev)
            : current_env(env), previous_env(std::move(prev)) {
        }

        ~EnvironmentGuard() {
            current_env = previous_env;
        }
    };

    void Interpreter::interpret(std::vector<std::unique_ptr<Stmt> > statements) {
        try {
            for (const auto &stmt: statements) {
                if (stmt) { execute(stmt.get()); }
            }
        } catch (const RuntimeError &error) {
            std::cerr << std::format("{}\n", error.what());
        }
        ast_storage.push_back(std::move(statements));
    }


    void Interpreter::execute(const Stmt *stmt) {
        if (const auto s = dynamic_cast<const UsingStmt *>(stmt)) return execute_using_stmt(s);
        if (const auto s = dynamic_cast<const TryCatchStmt *>(stmt)) return execute_try_catch_stmt(s);
        if (const auto s = dynamic_cast<const ExpressionStmt *>(stmt)) return execute_expression_stmt(s);
        if (const auto s = dynamic_cast<const PrintStmt *>(stmt)) return execute_print_stmt(s);
        if (const auto s = dynamic_cast<const PrintlnStmt *>(stmt)) return execute_print_ln_stmt(s);
        if (const auto s = dynamic_cast<const VarStmt *>(stmt)) return execute_var_stmt(s);
        if (const auto s = dynamic_cast<const BlockStmt *>(stmt)) return execute_block_stmt(s);
        if (const auto s = dynamic_cast<const IfStmt *>(stmt)) return execute_if_stmt(s);
        if (const auto s = dynamic_cast<const SwitchStmt *>(stmt)) return execute_switch_stmt(s);
        if (const auto s = dynamic_cast<const WhileStmt *>(stmt)) return execute_while_stmt(s);
        if (const auto s = dynamic_cast<const ForeachStmt *>(stmt)) return execute_foreach_stmt(s);
        if (const auto s = dynamic_cast<const BreakStmt *>(stmt)) return execute_break_stmt(s);
        if (const auto s = dynamic_cast<const ReturnStmt *>(stmt)) return execute_return_stmt(s);
        if (const auto s = dynamic_cast<const FunctionStmt *>(stmt)) return execute_function_stmt(s);
        throw std::runtime_error("Unknown statement type in interpreter.");
    }

    Value Interpreter::evaluate(const Expr *expr) {
        if (const auto e = dynamic_cast<const LiteralExpr *>(expr)) return evaluate_literal(e);
        if (const auto e = dynamic_cast<const VariableExpr *>(expr)) return evaluate_variable(e);
        if (const auto e = dynamic_cast<const BinaryExpr *>(expr)) return evaluate_binary(e);
        if (const auto e = dynamic_cast<const GroupingExpr *>(expr)) return evaluate_grouping(e);
        if (const auto e = dynamic_cast<const UnaryExpr *>(expr)) return evaluate_unary(e);
        if (const auto e = dynamic_cast<const UpdateExpr *>(expr)) return evaluate_update(e);
        if (const auto e = dynamic_cast<const AssignmentExpr *>(expr)) return evaluate_assignment(e);
        if (const auto e = dynamic_cast<const ArrayExpr *>(expr)) return evaluate_array(e);
        if (const auto e = dynamic_cast<const IndexExpr *>(expr)) return evaluate_index(e);
        if (const auto e = dynamic_cast<const IndexAssignmentExpr *>(expr)) return evaluate_index_assignment(e);
        if (const auto e = dynamic_cast<const LogicalExpr *>(expr)) return evaluate_logical(e);
        if (const auto e = dynamic_cast<const CallExpr *>(expr)) return evaluate_call(e);
        if (const auto e = dynamic_cast<const GetExpr *>(expr)) return evaluate_get(e);
        if (const auto e = dynamic_cast<const SetExpr *>(expr)) return evaluate_set(e);
        throw std::runtime_error("Unknown expression type in interpreter.");
    }

    void Interpreter::execute_function_stmt(const FunctionStmt *stmt) {
        auto function = std::make_shared<ObSLFunction>(stmt, environment);
        environment->define(std::string(stmt->name.lexeme), function);
    }

    Value Interpreter::evaluate_call(const CallExpr *expr) {
        const Value callee = evaluate(expr->callee.get());
        std::vector<Value> arguments;
        arguments.reserve(expr->arguments.size());
        for (const auto &arg: expr->arguments) {
            arguments.push_back(evaluate(arg.get()));
        }
        if (!std::holds_alternative<std::shared_ptr<ObSLCallable> >(callee)) {
            throw RuntimeError(expr->paren, "Can only call functions.");
        }
        const auto function = std::get<std::shared_ptr<ObSLCallable> >(callee);
        if (arguments.size() < static_cast<size_t>(function->min_arity()) || arguments.size() > static_cast<size_t>(
                function->arity())) {
            throw RuntimeError(expr->paren,
                               std::format("Expected between {} and {} arguments but got {}.", function->min_arity(),
                                           function->arity(), arguments.size()));
        }
        return function->call(this, arguments, expr->paren);
    }

    void Interpreter::execute_expression_stmt(const ExpressionStmt *stmt) {
        evaluate(stmt->expression.get());
    }

    void Interpreter::execute_print_stmt(const PrintStmt *stmt) {
        Value value = evaluate(stmt->expression.get());
        std::visit([this]<typename T0>(const T0 &arg) {
            using T = std::decay_t<T0>;
            if constexpr (std::is_same_v<T, std::monostate>) m_stdout << "null";
            else if constexpr (std::is_same_v<T, bool>) m_stdout << (arg ? "true" : "false");
            else if constexpr (std::is_same_v<T, double>) m_stdout << arg;
            else if constexpr (std::is_same_v<T, std::string>) m_stdout << arg;
            else if constexpr (std::is_same_v<T, std::shared_ptr<ObSLCallable> >) m_stdout << arg->to_string();
            else if constexpr (std::is_same_v<T, std::shared_ptr<ObSLArray> >) m_stdout << "[Array]";
            else if constexpr (std::is_same_v<T, std::shared_ptr<ObSLObject> >) m_stdout << "[Object]";
        }, value);
    }

    void Interpreter::execute_print_ln_stmt(const PrintlnStmt *stmt) {
        Value value = evaluate(stmt->expression.get());
        std::visit([this]<typename T0>(const T0 &arg) {
            using T = std::decay_t<T0>;
            if constexpr (std::is_same_v<T, std::monostate>) m_stdout << "null";
            else if constexpr (std::is_same_v<T, bool>) m_stdout << (arg ? "true" : "false");
            else if constexpr (std::is_same_v<T, double>) m_stdout << arg;
            else if constexpr (std::is_same_v<T, std::string>) m_stdout << arg;
            else if constexpr (std::is_same_v<T, std::shared_ptr<ObSLCallable> >) m_stdout << arg->to_string();
            else if constexpr (std::is_same_v<T, std::shared_ptr<ObSLArray> >) m_stdout << "[Array]";
            else if constexpr (std::is_same_v<T, std::shared_ptr<ObSLObject> >) m_stdout << "[Object]";
        }, value);
        m_stdout << "\n";
    }


    void Interpreter::execute_var_stmt(const VarStmt *stmt) {
        Value value = std::monostate{};
        if (stmt->initializer) {
            value = evaluate(stmt->initializer.get());
        }
        environment->define(std::string(stmt->name.lexeme), value);
    }

    Value Interpreter::evaluate_literal(const LiteralExpr *expr) {
        return expr->value;
    }

    Value Interpreter::evaluate_variable(const VariableExpr *expr) const {
        return environment->get(expr->name);
    }

    Value Interpreter::evaluate_array(const ArrayExpr *expr) {
        auto array = std::make_shared<ObSLArray>();
        array->elements.reserve(expr->elements.size());
        for (const auto &item: expr->elements) {
            array->elements.push_back(evaluate(item.get()));
        }
        return array;
    }

    Value Interpreter::evaluate_index(const IndexExpr *expr) {
        const Value callee = evaluate(expr->callee.get());
        const Value index_val = evaluate(expr->index.get());
        if (std::holds_alternative<std::shared_ptr<ObSLArray> >(callee)) {
            const auto array = std::get<std::shared_ptr<ObSLArray> >(callee);
            if (!std::holds_alternative<double>(index_val)) {
                throw RuntimeError(expr->bracket, "Array index must be a number.");
            }
            const int index = static_cast<int>(std::get<double>(index_val));
            if (index < 0 || index >= array->elements.size()) {
                throw RuntimeError(expr->bracket, "Array index out of bounds.");
            }
            return array->elements[index];
        }
        throw RuntimeError(expr->bracket, "Only arrays can be indexed.");
    }

    Value Interpreter::evaluate_index_assignment(const IndexAssignmentExpr *expr) {
        Value callee = evaluate(expr->callee.get());
        Value index_val = evaluate(expr->index.get());
        Value value = evaluate(expr->value.get());
        if (std::holds_alternative<std::shared_ptr<ObSLArray> >(callee)) {
            auto array = std::get<std::shared_ptr<ObSLArray> >(callee);

            if (!std::holds_alternative<double>(index_val)) {
                throw RuntimeError(expr->bracket, "Array index must be a number.");
            }

            int index = static_cast<int>(std::get<double>(index_val));

            if (index < 0 || index >= array->elements.size()) {
                throw RuntimeError(expr->bracket, "Array index out of bounds.");
            }
            array->elements[index] = value;
            return value;
        }
        throw RuntimeError(expr->bracket, "Only collections can be indexed for assignment.");
    }

    Value Interpreter::evaluate_binary(const BinaryExpr *expr) {
        const Value lhs = evaluate(expr->left.get());
        const Value rhs = evaluate(expr->right.get());
        switch (expr->oprt.type) {
            case TokenType::GREATER:
                check_number_operands(expr->oprt, lhs, rhs);
                return std::get<double>(lhs) > std::get<double>(rhs);
            case TokenType::GREATER_EQUAL:
                check_number_operands(expr->oprt, lhs, rhs);
                return std::get<double>(lhs) >= std::get<double>(rhs);
            case TokenType::LESS:
                check_number_operands(expr->oprt, lhs, rhs);
                return std::get<double>(lhs) < std::get<double>(rhs);
            case TokenType::LESS_EQUAL:
                check_number_operands(expr->oprt, lhs, rhs);
                return std::get<double>(lhs) <= std::get<double>(rhs);
            case TokenType::BANG_EQUAL:
                return !is_equal(lhs, rhs);
            case TokenType::EQUAL_EQUAL:
                return is_equal(lhs, rhs);
            case TokenType::MINUS:
                check_number_operands(expr->oprt, lhs, rhs);
                return std::get<double>(lhs) - std::get<double>(rhs);
            case TokenType::PLUS:
                if (std::holds_alternative<std::string>(lhs) || std::holds_alternative<std::string>(rhs)) {
                    auto stringify = [](const Value &val) -> std::string {
                        if (std::holds_alternative<std::string>(val)) return std::get<std::string>(val);
                        if (std::holds_alternative<double>(val)) return std::to_string(std::get<double>(val));
                        if (std::holds_alternative<bool>(val)) return std::get<bool>(val) ? "true" : "false";
                        return "null";
                    };
                    return stringify(lhs) + stringify(rhs);
                }
                if (std::holds_alternative<double>(lhs) && std::holds_alternative<double>(rhs)) {
                    return std::get<double>(lhs) + std::get<double>(rhs);
                }
                throw RuntimeError(expr->oprt, "Operands must be numbers or strings.");
            case TokenType::SLASH:
                check_number_operands(expr->oprt, lhs, rhs);
                if (std::get<double>(rhs) == 0) throw RuntimeError(expr->oprt, "Division by zero.");
                return std::get<double>(lhs) / std::get<double>(rhs);
            case TokenType::PERCENT:
                check_number_operands(expr->oprt, lhs, rhs);
                if (std::get<double>(rhs) == 0) throw RuntimeError(expr->oprt, "Modulo by zero.");
                return std::fmod(std::get<double>(lhs), std::get<double>(rhs));
            case TokenType::STAR:
                check_number_operands(expr->oprt, lhs, rhs);
                return std::get<double>(lhs) * std::get<double>(rhs);
            case TokenType::AMPERSAND:
                check_number_operands(expr->oprt, lhs, rhs);
                {
                    auto left = static_cast<int64_t>(std::get<double>(lhs));
                    auto right = static_cast<int64_t>(std::get<double>(rhs));
                    return static_cast<double>(left & right);
                }
            case TokenType::PIPE:
                check_number_operands(expr->oprt, lhs, rhs);
                {
                    auto left = static_cast<int64_t>(std::get<double>(lhs));
                    auto right = static_cast<int64_t>(std::get<double>(rhs));
                    return static_cast<double>(left | right);
                }
            case TokenType::CARET:
                check_number_operands(expr->oprt, lhs, rhs);
                {
                    auto left = static_cast<int64_t>(std::get<double>(lhs));
                    auto right = static_cast<int64_t>(std::get<double>(rhs));
                    return static_cast<double>(left ^ right);
                }
            case TokenType::LESS_LESS:
                check_number_operands(expr->oprt, lhs, rhs);
                {
                    auto left = static_cast<int64_t>(std::get<double>(lhs));
                    auto right = static_cast<int64_t>(std::get<double>(rhs));
                    return static_cast<double>(left << right);
                }
            case TokenType::GREATER_GREATER:
                check_number_operands(expr->oprt, lhs, rhs);
                {
                    auto left = static_cast<int64_t>(std::get<double>(lhs));
                    auto right = static_cast<int64_t>(std::get<double>(rhs));
                    return static_cast<double>(left >> right);
                }
            default:
                break;
        }
        return std::monostate{};
    }

    Value Interpreter::evaluate_grouping(const GroupingExpr *expr) {
        return evaluate(expr->expr.get());
    }

    Value Interpreter::evaluate_unary(const UnaryExpr *expr) {
        const Value right = evaluate(expr->right.get());
        switch (expr->oprt.type) {
            case TokenType::MINUS:
                check_number_operand(expr->oprt, right);
                return -std::get<double>(right);
            case TokenType::BANG:
                return !is_truthy(right);
            case TokenType::TILDE:
                if (std::holds_alternative<double>(right)) {
                    auto val = static_cast<int64_t>(std::get<double>(right));
                    return static_cast<double>(~val);
                }
                throw RuntimeError(expr->oprt, "Operand must be a number.");
            default:
                break;
        }
        return std::monostate{};
    }

    Value Interpreter::evaluate_update(const UpdateExpr *expr) {
        const Value current_value = environment->get(expr->name);
        check_number_operand(expr->oprt, current_value);
        double num = std::get<double>(current_value);
        double new_num = expr->oprt.type == TokenType::PLUS_PLUS ? num + 1.0 : num - 1.0;
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
        if (expr->oprt.type == TokenType::OR) {
            if (is_truthy(left)) return left;
        } else {
            if (!is_truthy(left)) return left;
        }
        return evaluate(expr->right.get());
    }

    bool Interpreter::is_truthy(const Value &value) {
        if (std::holds_alternative<std::monostate>(value)) return false;
        if (std::holds_alternative<bool>(value)) return std::get<bool>(value);
        return true;
    }

    void Interpreter::check_number_operand(const Token &oprt, const Value &oprnd) {
        if (std::holds_alternative<double>(oprnd)) return;
        throw RuntimeError(oprt, "Operand must be a number.");
    }

    void Interpreter::check_number_operands(const Token &oprt, const Value &lhs, const Value &rhs) {
        if (!std::holds_alternative<double>(lhs) || !std::holds_alternative<double>(rhs)) {
            throw RuntimeError(oprt, "Operands must be numbers.");
        }
    }

    bool Interpreter::is_equal(const Value &a, const Value &b) const {
        return std::visit([]<typename T0, typename T1>(const T0 &lhs, const T1 &rhs) -> bool {
            using L = std::decay_t<T0>;
            using R = std::decay_t<T1>;
            if constexpr (std::is_same_v<L, R>) {
                return lhs == rhs;
            } else {
                return false;
            }
        }, a, b);
    }


    void Interpreter::execute_using_stmt(const UsingStmt *stmt) {
        std::string module_name = std::filesystem::path(stmt->path).stem().string();
        if (loaded_modules.contains(stmt->path)) {
            environment->define(module_name, loaded_modules[stmt->path]);
            return;
        }
        std::ifstream file(stmt->path);
        if (!file.is_open()) {
            throw RuntimeError(stmt->keyword, std::format("Could not open module '{}'.", stmt->path));
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        source_storage.push_back(buffer.str());
        Lexer lexer(source_storage.back());
        auto tokens = lexer.tokenize();

        Parser parser(tokens);
        auto statements = parser.parse();

        auto module_env = std::make_shared<Environment>(globals);
        register_environment(module_env);
        auto previous_env = environment;

        try {
            environment = module_env;
            for (const auto &module_stmt: statements) {
                if (module_stmt) { execute(module_stmt.get()); }
            }
            environment = previous_env;
        } catch (...) {
            environment = previous_env;
            throw;
        }

        auto module_obj = std::make_shared<ObSLObject>();
        for (const auto &[name, val]: module_env->get_values()) {
            module_obj->fields[name] = val;
        }
        loaded_modules[stmt->path] = module_obj;
        environment->define(module_name, module_obj);
        ast_storage.push_back(std::move(statements));
    }

    void Interpreter::execute_try_catch_stmt(const TryCatchStmt *stmt) {
        try {
            execute_block_stmt(stmt->try_body.get());
        } catch (const RuntimeError &error) {
            auto catch_env = std::make_shared<Environment>(environment);
            register_environment(catch_env);
            catch_env->define(std::string(stmt->exception_var.lexeme), std::string(error.what()));
            execute_block(stmt->catch_body->statements, catch_env);
        }
    }


    void Interpreter::execute_block(const std::vector<std::unique_ptr<Stmt> > &statements,
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
        execute_block(stmt->statements, block_env);
    }

    void Interpreter::execute_if_stmt(const IfStmt *stmt) {
        if (is_truthy(evaluate(stmt->condition.get()))) {
            execute(stmt->then_branch.get());
        } else if (stmt->else_branch) {
            execute(stmt->else_branch.get());
        }
    }

    void Interpreter::execute_switch_stmt(const SwitchStmt *stmt) {
        // no fallthrough switch !!!
        const Value condition_val = evaluate(stmt->condition.get());
        try {
            const CaseBranch *default_branch = nullptr;
            // look for a specific case match
            for (const auto &case_branch: stmt->cases) {
                if (case_branch.match_value == nullptr) {
                    // save the default branch for later in case nothing
                    default_branch = &case_branch;
                    continue;
                }
                // match
                if (Value case_val = evaluate(case_branch.match_value.get()); is_equal(condition_val, case_val)) {
                    for (const auto &case_stmt: case_branch.statements) {
                        execute(case_stmt.get());
                    }
                    return;
                }
            }
            // no cases matched, run default
            if (default_branch != nullptr) {
                for (const auto &case_stmt: default_branch->statements) {
                    execute(case_stmt.get());
                }
            }
        } catch (const BreakException &) {
            // still catch the BreakException here just in case someone writes 'break;' out of habit
        }
    }


    void Interpreter::execute_while_stmt(const WhileStmt *stmt) {
        while (is_truthy(evaluate(stmt->condition.get()))) {
            try {
                execute(stmt->body.get());
            } catch (const BreakException &) {
                break;
            }
        }
    }

    void Interpreter::execute_foreach_stmt(const ForeachStmt *stmt) {
        if (const Value iterable_val = evaluate(stmt->iterable.get()); std::holds_alternative<std::shared_ptr<
            ObSLArray> >(iterable_val)) {
            const auto array = std::get<std::shared_ptr<ObSLArray> >(iterable_val);
            for (const auto &item: array->elements) {
                auto loop_env = std::make_shared<Environment>(environment);
                register_environment(loop_env);
                loop_env->define(std::string(stmt->loop_var.lexeme), item);
                EnvironmentGuard guard(environment, environment);
                environment = std::move(loop_env);
                try { execute(stmt->body.get()); } catch (const BreakException &) { break; }
            }
        } else {
            throw RuntimeError(stmt->loop_var, "Object is not iterable. Expected an Array.");
        }
    }

    void Interpreter::execute_break_stmt(const BreakStmt *) {
        throw BreakException();
    }

    void Interpreter::execute_return_stmt(const ReturnStmt *stmt) {
        Value value = std::monostate{};
        if (stmt->value) {
            value = evaluate(stmt->value.get());
        }
        throw ReturnException(value);
    }

    Value Interpreter::evaluate_get(const GetExpr *expr) {
        const Value obj = evaluate(expr->obj.get());

        // arrays
        if (std::holds_alternative<std::shared_ptr<ObSLArray> >(obj)) {
            const auto array = std::get<std::shared_ptr<ObSLArray> >(obj);
            if (expr->name.lexeme == "len") {
                return Value(static_cast<double>(array->elements.size()));
            }
            throw RuntimeError(expr->name, std::format("Undefined property '{}' on Array.", expr->name.lexeme));
        }

        // objects
        if (std::holds_alternative<std::shared_ptr<ObSLObject> >(obj)) {
            const auto instance = std::get<std::shared_ptr<ObSLObject> >(obj);
            // null terminated copy of property name
            const std::string prop_name(expr->name.lexeme);

            if (const auto it = instance->fields.find(prop_name); it != instance->fields.end()) {
                Value val = it->second;

                if (std::holds_alternative<std::shared_ptr<ObSLCallable> >(val)) {
                    auto callable = std::get<std::shared_ptr<ObSLCallable> >(val);
                    if (auto func = std::dynamic_pointer_cast<ObSLFunction>(callable)) {
                        return func->bind(instance, this);
                    }
                }
                return val;
            }
            throw RuntimeError(expr->name, std::format("Undefined property '{}' on object.", expr->name.lexeme));
        }
        throw RuntimeError(expr->name, " does not contain property.");
    }

    Value Interpreter::evaluate_set(const SetExpr *expr) {
        Value obj = evaluate(expr->obj.get());
        if (!std::holds_alternative<std::shared_ptr<ObSLObject> >(obj)) {
            throw RuntimeError(expr->name, "Only objects can have fields assigned.");
        }
        auto instance = std::get<std::shared_ptr<ObSLObject> >(obj);
        Value value = evaluate(expr->value.get());
        instance->fields[std::string(expr->name.lexeme)] = value;
        return value;
    }


    ObSLFunction::ObSLFunction(const FunctionStmt *declaration, std::shared_ptr<Environment> closure)
        : declaration(declaration), closure(std::move(closure)) {
    }

    int ObSLFunction::arity() const {
        return static_cast<int>(declaration->params.size());
    }

    int ObSLFunction::min_arity() const {
        int min_args = 0;
        for (const auto &param: declaration->params) {
            if (param.default_value == nullptr) {
                min_args++;
            }
        }
        return min_args;
    }

    Value ObSLFunction::call(Interpreter *interpreter, const std::vector<Value> &arguments, const Token &call_token) {
        size_t max_arity = declaration->params.size();
        size_t min_arity_val = min_arity();

        auto environment = std::make_shared<Environment>(closure);
        interpreter->register_environment(environment);
        auto previous_env = interpreter->get_current_environment();

        try {
            for (size_t i = 0; i < max_arity; ++i) {
                const auto &param = declaration->params[i];
                Value final_val;

                if (i < arguments.size()) {
                    final_val = arguments[i];
                } else {
                    interpreter->set_current_environment(environment);
                    final_val = interpreter->evaluate(param.default_value.get());
                }

                environment->define(std::string(param.name.lexeme), final_val);
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
        return std::format("<fn {}>", declaration->name.lexeme);
    }
} // namespace ObSL
