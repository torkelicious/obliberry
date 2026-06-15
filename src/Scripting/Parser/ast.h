#pragma once

#include <algorithm>
#include <memory>
#include <utility>
#include <variant>
#include <string_view>
#include <string>
#include <vector>
#include <format>
#include <unordered_map>
#include "Scripting/Lexer/Lexer.h"

namespace ObSL {
    class Interpreter;
    struct ObSLCallable;
    struct ObSLArray;
    struct ObSLObject;


    using Value = std::variant<
        std::monostate,
        bool,
        double,
        std::string,
        std::shared_ptr<ObSLCallable>,
        std::shared_ptr<ObSLArray>,
        std::shared_ptr<ObSLObject>
    >;

    struct ObSLObject {
        std::unordered_map<std::string, Value> fields;
    };

    struct ObSLArray {
        std::vector<Value> elements;
    };

    struct ObSLCallable {
        virtual ~ObSLCallable() = default;

        [[nodiscard]] virtual int arity() const = 0;

        virtual Value call(Interpreter *interpreter, const std::vector<Value> &arguments) = 0;

        [[nodiscard]] virtual std::string to_string() const { return "<callable>"; }
    };

    struct Expr {
        virtual ~Expr() = default;

        [[nodiscard]] virtual std::string to_string() const = 0;
    };

    struct Stmt {
        virtual ~Stmt() = default;

        [[nodiscard]] virtual std::string to_string() const = 0;
    };

    struct CallExpr : public Expr {
        std::unique_ptr<Expr> callee;
        Token paren;
        std::vector<std::unique_ptr<Expr> > arguments;

        CallExpr(std::unique_ptr<Expr> callee, const Token &paren, std::vector<std::unique_ptr<Expr> > arguments)
            : callee(std::move(callee)), paren(paren), arguments(std::move(arguments)) {
        }

        [[nodiscard]] std::string to_string() const override {
            std::string args;
            for (size_t i = 0; i < arguments.size(); ++i) {
                args += arguments[i]->to_string();
                if (i < arguments.size() - 1) args += ", ";
            }
            return std::format("(call {} [{}])", callee->to_string(), args);
        }
    };

    struct LiteralExpr : public Expr {
        Token token;
        Value value;

        explicit LiteralExpr(const Token &token, Value value)
            : token(token), value(std::move(value)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::visit([]<typename T0>(const T0 &arg) -> std::string {
                using T = std::decay_t<T0>;
                if constexpr (std::is_same_v<T, std::monostate>) return "null";
                else if constexpr (std::is_same_v<T, bool>) return arg ? std::string("true") : std::string("false");
                else if constexpr (std::is_same_v<T, double>) return std::format("{}", arg);
                else if constexpr (std::is_same_v<T, std::string>) return std::format("\"{}\"", arg);
                else if constexpr (std::is_same_v<T, std::shared_ptr<ObSLCallable> >)
                    return arg ? arg->to_string() : std::string("<callable>");
                else if constexpr (std::is_same_v<T, std::shared_ptr<ObSLArray> >)
                    return "[Array]";
                else if constexpr (std::is_same_v<T, std::shared_ptr<ObSLObject> >)
                    return "[Object]";
            }, value);
        }
    };

    struct BinaryExpr : public Expr {
        std::unique_ptr<Expr> left;
        Token oprt;
        std::unique_ptr<Expr> right;

        BinaryExpr(std::unique_ptr<Expr> left, const Token &oprt, std::unique_ptr<Expr> right)
            : left(std::move(left)), oprt(oprt), right(std::move(right)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("({} {} {})", oprt.lexeme, left->to_string(), right->to_string());
        }
    };

    struct LogicalExpr : public Expr {
        std::unique_ptr<Expr> left;
        Token oprt;
        std::unique_ptr<Expr> right;

        LogicalExpr(std::unique_ptr<Expr> left, const Token &oprt, std::unique_ptr<Expr> right)
            : left(std::move(left)), oprt(oprt), right(std::move(right)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("({} {} {})", oprt.lexeme, left->to_string(), right->to_string());
        }
    };

    struct GroupingExpr : public Expr {
        std::unique_ptr<Expr> expr;

        explicit GroupingExpr(std::unique_ptr<Expr> expr) : expr(std::move(expr)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("(group {})", expr->to_string());
        }
    };

    struct UnaryExpr : public Expr {
        Token oprt;
        std::unique_ptr<Expr> right;

        UnaryExpr(const Token &oprt, std::unique_ptr<Expr> right)
            : oprt(oprt), right(std::move(right)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("({} {})", oprt.lexeme, right->to_string());
        }
    };

    struct VariableExpr : public Expr {
        Token name;

        explicit VariableExpr(const Token &name) : name(name) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("{}", name.lexeme);
        }
    };

    struct AssignmentExpr : public Expr {
        Token name;
        std::unique_ptr<Expr> value;

        AssignmentExpr(const Token &name, std::unique_ptr<Expr> value)
            : name(name), value(std::move(value)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("(= {} {})", name.lexeme, value->to_string());
        }
    };

    struct UpdateExpr : public Expr {
        Token name;
        Token oprt;
        bool is_prefix;

        UpdateExpr(const Token &name, const Token &oprt, const bool is_prefix)
            : name(name), oprt(oprt), is_prefix(is_prefix) {
        }

        [[nodiscard]] std::string to_string() const override {
            if (is_prefix) return std::format("({}{})", oprt.lexeme, name.lexeme);
            return std::format("({}{})", name.lexeme, oprt.lexeme);
        }
    };

    struct ArrayExpr : public Expr {
        std::vector<std::unique_ptr<Expr> > elements;

        explicit ArrayExpr(std::vector<std::unique_ptr<Expr> > elements)
            : elements(std::move(elements)) {
        }

        [[nodiscard]] std::string to_string() const override {
            std::string elems;
            for (size_t i = 0; i < elements.size(); ++i) {
                elems += elements[i]->to_string();
                if (i < elements.size() - 1) elems += ", ";
            }
            return std::format("[{}]", elems);
        }
    };

    struct IndexExpr : Expr {
        std::unique_ptr<Expr> callee;
        Token bracket;
        std::unique_ptr<Expr> index;

        IndexExpr(std::unique_ptr<Expr> callee, const Token &bracket, std::unique_ptr<Expr> index)
            : callee(std::move(callee)), bracket(bracket), index(std::move(index)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("{}[{}]", callee->to_string(), index->to_string());
        }
    };

    struct IndexAssignmentExpr : public Expr {
        std::unique_ptr<Expr> callee;
        Token bracket;
        std::unique_ptr<Expr> index;
        std::unique_ptr<Expr> value;

        IndexAssignmentExpr(
            std::unique_ptr<Expr> callee,
            const Token &bracket,
            std::unique_ptr<Expr> index,
            std::unique_ptr<Expr> value)
            : callee(std::move(callee)), bracket(bracket), index(std::move(index)), value(std::move(value)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("(= {}[{}] {})", callee->to_string(), index->to_string(), value->to_string());
        }
    };

    struct GetExpr : public Expr {
        std::unique_ptr<Expr> obj;
        Token name;

        GetExpr(std::unique_ptr<Expr> obj, const Token &name)
            : obj(std::move(obj)), name(name) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("(. {} {})", obj->to_string(), name.lexeme);
        }
    };

    struct SetExpr : public Expr {
        std::unique_ptr<Expr> obj;
        Token name;
        std::unique_ptr<Expr> value;

        SetExpr(std::unique_ptr<Expr> obj, const Token &name, std::unique_ptr<Expr> value)
            : obj(std::move(obj)), name(name), value(std::move(value)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("(. {} {} {})", obj->to_string(), name.lexeme, value->to_string());
        }
    };

    struct UsingStmt : public Stmt {
        Token keyword;
        std::string path;

        UsingStmt(const Token &keyword, std::string path)
            : keyword(keyword), path(std::move(path)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("[UsingStmt: {}]\n", path);
        }
    };

    struct FunctionStmt : public Stmt {
        Token name;
        std::vector<Token> params;
        std::vector<std::unique_ptr<Stmt> > body;

        FunctionStmt(const Token &name, std::vector<Token> params, std::vector<std::unique_ptr<Stmt> > body)
            : name(name), params(std::move(params)), body(std::move(body)) {
        }

        [[nodiscard]] std::string to_string() const override {
            std::string param_str;
            for (size_t i = 0; i < params.size(); ++i) {
                param_str += params[i].lexeme;
                if (i < params.size() - 1) param_str += ", ";
            }
            return std::format("[FunctionStmt: (fn {}({}))]\n", name.lexeme, param_str);
        }
    };

    struct ExpressionStmt : public Stmt {
        std::unique_ptr<Expr> expression;

        explicit ExpressionStmt(std::unique_ptr<Expr> expr) : expression(std::move(expr)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("[ExprStmt: {}]\n", expression->to_string());
        }
    };

    struct PrintStmt : public Stmt {
        Token keyword;
        std::unique_ptr<Expr> expression;

        PrintStmt(const Token &keyword, std::unique_ptr<Expr> expr)
            : keyword(keyword), expression(std::move(expr)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("[PrintStmt: {}]\n", expression->to_string());
        }
    };

    struct PrintlnStmt : public Stmt {
        Token keyword;
        std::unique_ptr<Expr> expression;

        PrintlnStmt(const Token &keyword, std::unique_ptr<Expr> expr)
            : keyword(keyword), expression(std::move(expr)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("[PrintlnStmt : {}]\n", expression->to_string());
        }
    };


    struct BlockStmt : public Stmt {
        std::vector<std::unique_ptr<Stmt> > statements;

        explicit BlockStmt(std::vector<std::unique_ptr<Stmt> > stmts) : statements(std::move(stmts)) {
        }

        [[nodiscard]] std::string to_string() const override {
            std::string body;
            for (const auto &inner_stmt: statements) {
                body += std::format("  {}", inner_stmt->to_string());
            }
            return std::format("[BlockStmt: {{\n{}}}]\n", body);
        }
    };

    struct IfStmt : public Stmt {
        std::unique_ptr<Expr> condition;
        std::unique_ptr<Stmt> then_branch;
        std::unique_ptr<Stmt> else_branch;

        IfStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> then_branch, std::unique_ptr<Stmt> else_branch)
            : condition(std::move(condition)), then_branch(std::move(then_branch)),
              else_branch(std::move(else_branch)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("[IfStmt: (if {}\n  then: {}  {})]\n",
                               condition->to_string(),
                               then_branch->to_string(),
                               else_branch ? std::format("else: {}", else_branch->to_string()) : ""
            );
        }
    };


    struct CaseBranch {
        std::unique_ptr<Expr> match_value;
        std::vector<std::unique_ptr<Stmt> > statements;

        CaseBranch(std::unique_ptr<Expr> match_value, std::vector<std::unique_ptr<Stmt> > statements)
            : match_value(std::move(match_value)), statements(std::move(statements)) {
        }
    };

    struct SwitchStmt : public Stmt {
        std::unique_ptr<Expr> condition;
        std::vector<CaseBranch> cases;

        SwitchStmt(
            std::unique_ptr<Expr> condition,
            std::vector<CaseBranch> cases) : condition(std::move(condition)), cases(std::move(cases)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("[SwitchStmt: on {}]\n", condition->to_string());
        }
    };


    struct WhileStmt : public Stmt {
        std::unique_ptr<Expr> condition;
        std::unique_ptr<Stmt> body;

        WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body)
            : condition(std::move(condition)), body(std::move(body)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("[WhileStmt: (while {}\n  body: {})]\n", condition->to_string(), body->to_string());
        }
    };

    struct ForeachStmt : public Stmt {
        Token loop_var;
        std::unique_ptr<Expr> iterable;
        std::unique_ptr<Stmt> body;

        ForeachStmt(const Token &loop_var, std::unique_ptr<Expr> iterable, std::unique_ptr<Stmt> body)
            : loop_var(loop_var), iterable(std::move(iterable)), body(std::move(body)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("[ForeachStmt: (foreach {} in {})\n  body: {}]\n",
                               loop_var.lexeme, iterable->to_string(), body->to_string());
        }
    };

    struct ReturnStmt : public Stmt {
        Token keyword;
        std::unique_ptr<Expr> value;

        ReturnStmt(const Token &keyword, std::unique_ptr<Expr> value)
            : keyword(keyword), value(std::move(value)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("[ReturnStmt: (return{})]\n", value ? std::format(" {}", value->to_string()) : "");
        }
    };

    struct BreakStmt : public Stmt {
        Token keyword;

        explicit BreakStmt(const Token &keyword) : keyword(keyword) {
        }

        [[nodiscard]] std::string to_string() const override {
            return "[BreakStmt]\n";
        }
    };

    struct VarStmt : public Stmt {
        Token name;
        std::unique_ptr<Expr> initializer;

        VarStmt(const Token &name, std::unique_ptr<Expr> initializer)
            : name(name), initializer(std::move(initializer)) {
        }

        [[nodiscard]] std::string to_string() const override {
            return std::format("[VarStmt: (var {}{})]\n",
                               name.lexeme,
                               initializer ? std::format(" = {}", initializer->to_string()) : ""
            );
        }
    };
} // namespace ObSL
