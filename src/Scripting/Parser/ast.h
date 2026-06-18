#pragma once

#include <algorithm>
#include <memory>
#include <utility>
#include <variant>
#include <string_view>
#include <string>
#include <vector>
#include <format>
#include <ranges>
#include <unordered_map>

#include "ast.h"
#include "Scripting/Lexer/Lexer.h"
#include "Scripting/GarbageCollector.h"

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
        ObSLCallable *,
        ObSLArray *,
        ObSLObject *>;


    void mark_value(const Value &val);

    struct ObSLObject : public GCObject {
        std::unordered_map<std::string, Value> fields;

        void mark() override {
            if (is_marked)return;
            is_marked = true;
            for (auto &val: fields | std::views::values) {
                mark_value(val);
            }
        }
    };

    struct ObSLArray : public GCObject {
        std::vector<Value> elements;

        void mark() override {
            if (is_marked)return;
            is_marked = true;
            for (auto &val: elements) {
                mark_value(val);
            }
        }
    };

    struct ObSLCallable : public GCObject {
        ~ObSLCallable() override = default;

        [[nodiscard]] virtual int arity() const = 0;

        [[nodiscard]] virtual int min_arity() const { return arity(); }

        virtual Value call(Interpreter *interpreter, const std::vector<Value> &arguments, const Token &call_token) = 0;

        [[nodiscard]] virtual std::string to_string() const { return "<callable>"; }

        void mark() override {
        }
    };

    enum class ExprType : uint8_t {
        Call, Literal, Binary, Logical, Grouping, Unary, Variable,
        Assignment, Update, Array, Index, IndexAssignment, Get, Set
    };

    enum class StmtType : uint8_t {
        Using, Expression, Print, Println, Block, Function, If,
        Switch, While, Foreach, Return, Break, Var, TryCatch, Struct
    };

    // expression base
    struct Expr {
        virtual ~Expr() = default;

        [[nodiscard]] virtual ExprType type() const noexcept = 0;

        [[nodiscard]] virtual std::string to_string() const = 0;
    };

    // statement base
    struct Stmt {
        virtual ~Stmt() = default;

        [[nodiscard]] virtual StmtType type() const noexcept = 0;

        [[nodiscard]] virtual std::string to_string() const = 0;
    };

    // forward declarations
    // exprs
    struct CallExpr;
    struct LiteralExpr;
    struct BinaryExpr;
    struct LogicalExpr;
    struct GroupingExpr;
    struct UnaryExpr;
    struct VariableExpr;
    struct AssignmentExpr;
    struct UpdateExpr;
    struct ArrayExpr;
    struct IndexExpr;
    struct IndexAssignmentExpr;
    struct GetExpr;
    struct SetExpr;
    // stmts
    struct UsingStmt;
    struct ExpressionStmt;
    struct PrintStmt;
    struct PrintlnStmt;
    struct BlockStmt;
    struct FunctionStmt;
    struct IfStmt;
    struct SwitchStmt;
    struct WhileStmt;
    struct ForeachStmt;
    struct ReturnStmt;
    struct BreakStmt;
    struct VarStmt;
    struct TryCatchStmt;
    struct StructStmt;

    // Expressions

    struct CallExpr : public Expr {
        std::unique_ptr<Expr> callee;
        Token paren;
        std::vector<std::unique_ptr<Expr> > arguments;

        CallExpr(std::unique_ptr<Expr> callee, const Token &paren, std::vector<std::unique_ptr<Expr> > arguments)
            : callee(std::move(callee)), paren(paren), arguments(std::move(arguments)) {
        }

        [[nodiscard]] ExprType type() const noexcept override { return ExprType::Call; }

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

        [[nodiscard]] ExprType type() const noexcept override { return ExprType::Literal; }

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
                return "";
            }, value);
        }
    };

    struct BinaryExpr : public Expr {
        std::unique_ptr<Expr> left;
        TokenType oprt_type;
        std::unique_ptr<Expr> right;

        BinaryExpr(std::unique_ptr<Expr> left, const Token &oprt, std::unique_ptr<Expr> right)
            : left(std::move(left)), oprt_type(oprt.type), right(std::move(right)) {
        }

        [[nodiscard]] ExprType type() const noexcept override { return ExprType::Binary; }

        [[nodiscard]] std::string to_string() const override {
            return std::format("({} {} {})", static_cast<int>(oprt_type), left->to_string(), right->to_string());
        }
    };

    struct LogicalExpr : public Expr {
        std::unique_ptr<Expr> left;
        TokenType oprt_type;
        std::unique_ptr<Expr> right;

        LogicalExpr(std::unique_ptr<Expr> left, const Token &oprt, std::unique_ptr<Expr> right)
            : left(std::move(left)), oprt_type(oprt.type), right(std::move(right)) {
        }

        [[nodiscard]] ExprType type() const noexcept override { return ExprType::Logical; }

        [[nodiscard]] std::string to_string() const override {
            return std::format("({} {} {})", static_cast<int>(oprt_type), left->to_string(), right->to_string());
        }
    };

    struct GroupingExpr : public Expr {
        std::unique_ptr<Expr> expr;

        explicit GroupingExpr(std::unique_ptr<Expr> expr) : expr(std::move(expr)) {
        }

        [[nodiscard]] ExprType type() const noexcept override { return ExprType::Grouping; }

        [[nodiscard]] std::string to_string() const override {
            return std::format("(group {})", expr->to_string());
        }
    };

    struct UnaryExpr : public Expr {
        TokenType oprt_type;
        std::unique_ptr<Expr> right;

        UnaryExpr(const Token &oprt, std::unique_ptr<Expr> right)
            : oprt_type(oprt.type), right(std::move(right)) {
        }

        [[nodiscard]] ExprType type() const noexcept override { return ExprType::Unary; }

        [[nodiscard]] std::string to_string() const override {
            return std::format("({} {})", static_cast<int>(oprt_type), right->to_string());
        }
    };

    struct VariableExpr : public Expr {
        std::string_view name;

        explicit VariableExpr(const Token &name) : name(name.lexeme) {
        }

        explicit VariableExpr(const std::string_view name) : name(name) {
        }

        [[nodiscard]] ExprType type() const noexcept override { return ExprType::Variable; }

        [[nodiscard]] std::string to_string() const override {
            return std::format("{}", name);
        }
    };

    struct AssignmentExpr : public Expr {
        std::string_view name;
        std::unique_ptr<Expr> value;

        AssignmentExpr(const Token &name, std::unique_ptr<Expr> value)
            : name(name.lexeme), value(std::move(value)) {
        }

        AssignmentExpr(const std::string_view name, std::unique_ptr<Expr> value)
            : name(name), value(std::move(value)) {
        }

        [[nodiscard]] ExprType type() const noexcept override { return ExprType::Assignment; }

        [[nodiscard]] std::string to_string() const override {
            return std::format("(= {} {})", name, value->to_string());
        }
    };

    struct UpdateExpr : public Expr {
        std::string_view name;
        TokenType oprt_type;
        uint8_t is_prefix: 1; // bit field for boolean

        UpdateExpr(const std::string_view name, const Token &oprt, const bool prefix)
            : name(name), oprt_type(oprt.type), is_prefix(prefix) {
        }

        UpdateExpr(const Token &name, const Token &oprt, const bool prefix)
            : name(name.lexeme), oprt_type(oprt.type), is_prefix(prefix) {
        }

        [[nodiscard]] ExprType type() const noexcept override { return ExprType::Update; }

        [[nodiscard]] std::string to_string() const override {
            std::string oprt_str;
            switch (oprt_type) {
                case TokenType::PLUS_PLUS: oprt_str = "++";
                    break;
                case TokenType::MINUS_MINUS: oprt_str = "--";
                    break;
                default: oprt_str = "??";
                    break;
            }
            if (is_prefix) return std::format("({}{})", oprt_str, name);
            return std::format("({}{})", name, oprt_str);
        }
    };

    struct ArrayExpr : public Expr {
        std::vector<std::unique_ptr<Expr> > elements;

        explicit ArrayExpr(std::vector<std::unique_ptr<Expr> > elements)
            : elements(std::move(elements)) {
        }

        [[nodiscard]] ExprType type() const noexcept override { return ExprType::Array; }

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

        [[nodiscard]] ExprType type() const noexcept override { return ExprType::Index; }

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

        [[nodiscard]] ExprType type() const noexcept override { return ExprType::IndexAssignment; }

        [[nodiscard]] std::string to_string() const override {
            return std::format("(= {}[{}] {})", callee->to_string(), index->to_string(), value->to_string());
        }
    };

    struct GetExpr : public Expr {
        std::unique_ptr<Expr> obj;
        std::string_view name;

        GetExpr(std::unique_ptr<Expr> obj, const Token &name)
            : obj(std::move(obj)), name(name.lexeme) {
        }

        [[nodiscard]] ExprType type() const noexcept override { return ExprType::Get; }

        [[nodiscard]] std::string to_string() const override {
            return std::format("(. {} {})", obj->to_string(), name);
        }
    };

    struct SetExpr : public Expr {
        std::unique_ptr<Expr> obj;
        std::string_view name;
        std::unique_ptr<Expr> value;

        SetExpr(std::unique_ptr<Expr> obj, const std::string_view name, std::unique_ptr<Expr> value)
            : obj(std::move(obj)), name(name), value(std::move(value)) {
        }

        SetExpr(std::unique_ptr<Expr> obj, const Token &name, std::unique_ptr<Expr> value)
            : obj(std::move(obj)), name(name.lexeme), value(std::move(value)) {
        }

        [[nodiscard]] ExprType type() const noexcept override { return ExprType::Set; }

        [[nodiscard]] std::string to_string() const override {
            return std::format("(. {} {} {})", obj->to_string(), name, value->to_string());
        }
    };

    // Statements

    struct UsingStmt : public Stmt {
        std::string path;

        UsingStmt(const Token &/*keyword*/, std::string path)
            : path(std::move(path)) {
        }

        [[nodiscard]] StmtType type() const noexcept override { return StmtType::Using; }

        [[nodiscard]] std::string to_string() const override {
            return std::format("[UsingStmt: {}]\n", path);
        }
    };

    struct ExpressionStmt : public Stmt {
        std::unique_ptr<Expr> expression;

        explicit ExpressionStmt(std::unique_ptr<Expr> expr) : expression(std::move(expr)) {
        }

        [[nodiscard]] StmtType type() const noexcept override { return StmtType::Expression; }

        [[nodiscard]] std::string to_string() const override {
            return std::format("[ExprStmt: {}]\n", expression->to_string());
        }
    };

    struct PrintStmt : public Stmt {
        std::unique_ptr<Expr> expression;

        PrintStmt(const Token &/*keyword*/, std::unique_ptr<Expr> expr)
            : expression(std::move(expr)) {
        }

        [[nodiscard]] StmtType type() const noexcept override { return StmtType::Print; }

        [[nodiscard]] std::string to_string() const override {
            return std::format("[PrintStmt: {}]\n", expression->to_string());
        }
    };

    struct PrintlnStmt : public Stmt {
        std::unique_ptr<Expr> expression;

        PrintlnStmt(const Token &/*keyword*/, std::unique_ptr<Expr> expr)
            : expression(std::move(expr)) {
        }

        [[nodiscard]] StmtType type() const noexcept override { return StmtType::Println; }

        [[nodiscard]] std::string to_string() const override {
            return std::format("[PrintlnStmt : {}]\n", expression->to_string());
        }
    };

    struct BlockStmt : public Stmt {
        std::vector<std::unique_ptr<Stmt> > statements;

        explicit BlockStmt(std::vector<std::unique_ptr<Stmt> > stmts) : statements(std::move(stmts)) {
        }

        [[nodiscard]] StmtType type() const noexcept override { return StmtType::Block; }

        [[nodiscard]] std::string to_string() const override {
            std::string body;
            for (const auto &inner_stmt: statements) {
                body += std::format("  {}", inner_stmt->to_string());
            }
            return std::format("[BlockStmt: {{\n{}}}\n", body);
        }
    };

    struct Param {
        std::string_view name;
        std::unique_ptr<Expr> default_value;

        Param() = default;

        Param(const Token &name, std::unique_ptr<Expr> default_value)
            : name(name.lexeme), default_value(std::move(default_value)) {
        }
    };

    struct FunctionStmt : public Stmt {
        std::string_view name;
        std::vector<Param> params;
        std::unique_ptr<BlockStmt> body;

        FunctionStmt(const Token &name, std::vector<Param> params, std::unique_ptr<BlockStmt> body)
            : name(name.lexeme), params(std::move(params)), body(std::move(body)) {
        }

        [[nodiscard]] StmtType type() const noexcept override { return StmtType::Function; }

        [[nodiscard]] std::string to_string() const override {
            return std::format("[FunctionStmt: {}]", name);
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

        [[nodiscard]] StmtType type() const noexcept override { return StmtType::If; }

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

        [[nodiscard]] StmtType type() const noexcept override { return StmtType::Switch; }

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

        [[nodiscard]] StmtType type() const noexcept override { return StmtType::While; }

        [[nodiscard]] std::string to_string() const override {
            return std::format("[WhileStmt: (while {}\n  body: {})]\n", condition->to_string(), body->to_string());
        }
    };

    struct ForeachStmt : public Stmt {
        std::string_view loop_var;
        std::unique_ptr<Expr> iterable;
        std::unique_ptr<Stmt> body;

        ForeachStmt(const Token &loop_var, std::unique_ptr<Expr> iterable, std::unique_ptr<Stmt> body)
            : loop_var(loop_var.lexeme), iterable(std::move(iterable)), body(std::move(body)) {
        }

        [[nodiscard]] StmtType type() const noexcept override { return StmtType::Foreach; }

        [[nodiscard]] std::string to_string() const override {
            return std::format("[ForeachStmt: (foreach {} in {})\n  body: {}]\n",
                               loop_var, iterable->to_string(), body->to_string());
        }
    };

    struct ReturnStmt : public Stmt {
        std::unique_ptr<Expr> value;

        ReturnStmt(const Token &/*keyword*/, std::unique_ptr<Expr> value)
            : value(std::move(value)) {
        }

        [[nodiscard]] StmtType type() const noexcept override { return StmtType::Return; }

        [[nodiscard]] std::string to_string() const override {
            return std::format("[ReturnStmt: (return{})]\n", value ? std::format(" {}", value->to_string()) : "");
        }
    };

    struct BreakStmt : public Stmt {
        explicit BreakStmt(const Token &/*keyword*/) {
        }

        [[nodiscard]] StmtType type() const noexcept override { return StmtType::Break; }

        [[nodiscard]] std::string to_string() const override {
            return "[BreakStmt]\n";
        }
    };

    struct VarStmt : public Stmt {
        std::string_view name;
        std::unique_ptr<Expr> initializer;

        VarStmt(const Token &name, std::unique_ptr<Expr> initializer)
            : name(name.lexeme), initializer(std::move(initializer)) {
        }

        [[nodiscard]] StmtType type() const noexcept override { return StmtType::Var; }

        [[nodiscard]] std::string to_string() const override {
            return std::format("[VarStmt: (var {}{})]\n",
                               name,
                               initializer ? std::format(" = {}", initializer->to_string()) : ""
            );
        }
    };

    struct TryCatchStmt : public Stmt {
        std::unique_ptr<BlockStmt> try_body;
        std::string_view exception_var;
        std::unique_ptr<BlockStmt> catch_body;

        TryCatchStmt(std::unique_ptr<BlockStmt> try_body, const Token &exception_var,
                     std::unique_ptr<BlockStmt> catch_body)
            : try_body(std::move(try_body)), exception_var(exception_var.lexeme), catch_body(std::move(catch_body)) {
        }

        [[nodiscard]] StmtType type() const noexcept override { return StmtType::TryCatch; }

        [[nodiscard]] std::string to_string() const override {
            return std::format("[TryCatchStmt: try {} catch({}) {}]\n",
                               try_body->to_string(), exception_var, catch_body->to_string());
        }
    };

    struct StructField {
        Token name;
        std::unique_ptr<Expr> default_value;
    };

    struct StructStmt : public Stmt {
        Token name;
        std::vector<StructField> fields;

        StructStmt(const Token &name, std::vector<StructField> fields)
            : name(name), fields(std::move(fields)) {
        }

        [[nodiscard]] StmtType type() const noexcept override { return StmtType::Struct; }
        [[nodiscard]] std::string to_string() const override { return std::format("[StructStmt: {}]\n", name.lexeme); }
    };

    struct ObSLStruct : public ObSLCallable {
        const StructStmt *declaration;

        ObSLStruct(const StructStmt *declaration) : declaration(declaration) {
        }

        void mark() override {
            is_marked = true;
        }

        [[nodiscard]] int arity() const override { return declaration->fields.size(); }

        [[nodiscard]] int min_arity() const override {
            return 0;
        }


        Value call(Interpreter *interpreter, const std::vector<Value> &arguments, const Token &call_token) override;

        [[nodiscard]] std::string to_string() const override {
            return std::format("<struct {}>", declaration->name.lexeme);
        }
    };
} // namespace ObSL
