#include "ASTDeserializer.h"

namespace ObSL {
    // helper utilities

    static Token deserialize_token(ASTDeserializer &deser) {
        Token token;
        token.type = deser.read<TokenType>();
        token.lexeme = deser.read_string_view();
        token.line = deser.read<uint16_t>();
        token.column = deser.read<uint16_t>();
        token.start_pos = deser.read<uint32_t>();
        token.end_pos = deser.read<uint32_t>();
        return token;
    }

    static Value deserialize_value(ASTDeserializer &deser) {
        const uint8_t discriminant = deser.read<uint8_t>();

        switch (discriminant) {
            case 0: // monostate
                return std::monostate{};
            case 1: // bool
                return static_cast<bool>(deser.read<uint8_t>());
            case 2: // double
                return deser.read<double>();
            case 3: // string
                return std::string(deser.read_string_view());
            default:
                throw std::runtime_error("Malformed binary AST: Unknown value discriminant");
        }
    }

    static Param deserialize_param(ASTDeserializer &deser) {
        Param param;
        param.name = deser.read_string_view();
        param.default_value = deser.deserialize_expr();
        return param;
    }

    static StructField deserialize_struct_field(ASTDeserializer &deser) {
        StructField field;
        field.name = deserialize_token(deser);
        field.default_value = deser.deserialize_expr();
        return field;
    }

    static CaseBranch deserialize_case_branch(ASTDeserializer &deser) {
        auto match_value = deser.deserialize_expr();
        const uint32_t count = deser.read<uint32_t>();
        std::vector<std::unique_ptr<Stmt> > statements;
        statements.reserve(count);
        for (uint32_t i = 0; i < count; ++i) {
            statements.push_back(deser.deserialize_stmt());
        }
        return CaseBranch(std::move(match_value), std::move(statements));
    }

    // expression deserialization

    std::unique_ptr<Expr> ASTDeserializer::deserialize_expr() {
        const uint8_t tag = read<uint8_t>();

        // null pointer
        if (tag == 0xFF) {
            return nullptr;
        }

        const auto etype = static_cast<ExprType>(tag);

        switch (etype) {
            case ExprType::Call: {
                auto callee = deserialize_expr();
                Token paren = deserialize_token(*this);
                const uint32_t argc = read<uint32_t>();
                std::vector<std::unique_ptr<Expr> > args;
                args.reserve(argc);
                for (uint32_t i = 0; i < argc; ++i) {
                    args.push_back(deserialize_expr());
                }
                return std::make_unique<CallExpr>(std::move(callee), paren, std::move(args));
            }
            case ExprType::Literal: {
                Token token = deserialize_token(*this);
                Value value = deserialize_value(*this);
                return std::make_unique<LiteralExpr>(token, std::move(value));
            }
            case ExprType::Binary: {
                auto left = deserialize_expr();
                const TokenType oprt = read<TokenType>();
                auto right = deserialize_expr();
                Token dummy_oprt{};
                dummy_oprt.type = oprt;
                return std::make_unique<BinaryExpr>(std::move(left), dummy_oprt, std::move(right));
            }
            case ExprType::Logical: {
                auto left = deserialize_expr();
                const TokenType oprt = read<TokenType>();
                auto right = deserialize_expr();
                Token dummy_oprt{};
                dummy_oprt.type = oprt;
                return std::make_unique<LogicalExpr>(std::move(left), dummy_oprt, std::move(right));
            }
            case ExprType::Grouping: {
                auto expr = deserialize_expr();
                return std::make_unique<GroupingExpr>(std::move(expr));
            }
            case ExprType::Unary: {
                const TokenType oprt = read<TokenType>();
                auto right = deserialize_expr();
                Token dummy_oprt{};
                dummy_oprt.type = oprt;
                return std::make_unique<UnaryExpr>(dummy_oprt, std::move(right));
            }
            case ExprType::Variable: {
                const std::string_view name = read_string_view();
                return std::make_unique<VariableExpr>(name);
            }
            case ExprType::Assignment: {
                const std::string_view name = read_string_view();
                auto value = deserialize_expr();
                return std::make_unique<AssignmentExpr>(name, std::move(value));
            }
            case ExprType::Update: {
                const std::string_view name = read_string_view();
                const TokenType oprt = read<TokenType>();
                const bool is_prefix = read<uint8_t>() != 0;
                Token dummy_oprt{};
                dummy_oprt.type = oprt;
                return std::make_unique<UpdateExpr>(name, dummy_oprt, is_prefix);
            }
            case ExprType::Array: {
                const uint32_t count = read<uint32_t>();
                std::vector<std::unique_ptr<Expr> > elements;
                elements.reserve(count);
                for (uint32_t i = 0; i < count; ++i) {
                    elements.push_back(deserialize_expr());
                }
                return std::make_unique<ArrayExpr>(std::move(elements));
            }
            case ExprType::Index: {
                auto callee = deserialize_expr();
                Token bracket = deserialize_token(*this);
                auto index = deserialize_expr();
                return std::make_unique<IndexExpr>(std::move(callee), bracket, std::move(index));
            }
            case ExprType::IndexAssignment: {
                auto callee = deserialize_expr();
                Token bracket = deserialize_token(*this);
                auto index = deserialize_expr();
                auto value = deserialize_expr();
                return std::make_unique<IndexAssignmentExpr>(
                    std::move(callee), bracket, std::move(index), std::move(value));
            }
            case ExprType::Get: {
                auto obj = deserialize_expr();
                const std::string_view name = read_string_view();
                Token dummy_name{};
                dummy_name.lexeme = name;
                return std::make_unique<GetExpr>(std::move(obj), dummy_name);
            }
            case ExprType::Set: {
                auto obj = deserialize_expr();
                const std::string_view name = read_string_view();
                auto value = deserialize_expr();
                return std::make_unique<SetExpr>(std::move(obj), name, std::move(value));
            }
            case ExprType::TypeCheck: {
                auto left = deserialize_expr();
                const std::string type_name(read_string_view());
                return std::make_unique<TypeCheckExpr>(std::move(left), std::move(type_name));
            }
        }

        throw std::runtime_error("Malformed binary AST: Unknown expression type");
    }

    // statement deserialization

    std::unique_ptr<Stmt> ASTDeserializer::deserialize_stmt() {
        const uint8_t tag = read<uint8_t>();

        if (tag == 0xFF) {
            return nullptr;
        }

        const auto stype = static_cast<StmtType>(tag);

        switch (stype) {
            case StmtType::Using: {
                const std::string path(read_string_view());
                Token dummy{};
                dummy.type = TokenType::USING;
                return std::make_unique<UsingStmt>(dummy, std::move(path));
            }
            case StmtType::Expression: {
                auto expr = deserialize_expr();
                return std::make_unique<ExpressionStmt>(std::move(expr));
            }
            case StmtType::Print: {
                auto expr = deserialize_expr();
                Token dummy{};
                dummy.type = TokenType::PRINT;
                return std::make_unique<PrintStmt>(dummy, std::move(expr));
            }
            case StmtType::Println: {
                auto expr = deserialize_expr();
                Token dummy{};
                dummy.type = TokenType::PRINTLN;
                return std::make_unique<PrintlnStmt>(dummy, std::move(expr));
            }
            case StmtType::Block: {
                const uint32_t count = read<uint32_t>();
                std::vector<std::unique_ptr<Stmt> > stmts;
                stmts.reserve(count);
                for (uint32_t i = 0; i < count; ++i) {
                    stmts.push_back(deserialize_stmt());
                }
                return std::make_unique<BlockStmt>(std::move(stmts));
            }
            case StmtType::Function: {
                const std::string_view name = read_string_view();
                const uint32_t param_count = read<uint32_t>();
                std::vector<Param> params;
                params.reserve(param_count);
                for (uint32_t i = 0; i < param_count; ++i) {
                    params.push_back(deserialize_param(*this));
                }
                // FunctionStmt expects the body as BlockStmt so downcast after deserialisation.
                auto body = deserialize_stmt();
                auto block_body = std::unique_ptr<BlockStmt>(
                    static_cast<BlockStmt *>(body.release()));
                Token dummy_name{};
                dummy_name.lexeme = name;
                return std::make_unique<FunctionStmt>(dummy_name, std::move(params), std::move(block_body));
            }
            case StmtType::If: {
                auto condition = deserialize_expr();
                auto then_branch = deserialize_stmt();
                auto else_branch = deserialize_stmt();
                return std::make_unique<IfStmt>(
                    std::move(condition), std::move(then_branch), std::move(else_branch));
            }
            case StmtType::Switch: {
                auto condition = deserialize_expr();
                const uint32_t case_count = read<uint32_t>();
                std::vector<CaseBranch> cases;
                cases.reserve(case_count);
                for (uint32_t i = 0; i < case_count; ++i) {
                    cases.push_back(deserialize_case_branch(*this));
                }
                return std::make_unique<SwitchStmt>(std::move(condition), std::move(cases));
            }
            case StmtType::While: {
                auto condition = deserialize_expr();
                auto body = deserialize_stmt();
                return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
            }
            case StmtType::Foreach: {
                const std::string_view loop_var = read_string_view();
                auto iterable = deserialize_expr();
                auto body = deserialize_stmt();
                Token dummy_var{};
                dummy_var.lexeme = loop_var;
                return std::make_unique<ForeachStmt>(dummy_var, std::move(iterable), std::move(body));
            }
            case StmtType::Return: {
                auto value = deserialize_expr();
                Token dummy{};
                dummy.type = TokenType::RETURN;
                return std::make_unique<ReturnStmt>(dummy, std::move(value));
            }
            case StmtType::Break: {
                Token dummy{};
                dummy.type = TokenType::BREAK;
                return std::make_unique<BreakStmt>(dummy);
            }
            case StmtType::Var: {
                const std::string_view name = read_string_view();
                auto initializer = deserialize_expr();
                Token dummy_name{};
                dummy_name.lexeme = name;
                return std::make_unique<VarStmt>(dummy_name, std::move(initializer));
            }
            case StmtType::TryCatch: {
                auto try_body = deserialize_stmt();
                const std::string_view exception_var = read_string_view();
                auto catch_body = deserialize_stmt();
                // TryCatchStmt expects BlockStmt for both bodies
                auto try_block = std::unique_ptr<BlockStmt>(
                    static_cast<BlockStmt *>(try_body.release()));
                auto catch_block = std::unique_ptr<BlockStmt>(
                    static_cast<BlockStmt *>(catch_body.release()));
                Token dummy_exc{};
                dummy_exc.lexeme = exception_var;
                return std::make_unique<TryCatchStmt>(
                    std::move(try_block), dummy_exc, std::move(catch_block));
            }
            case StmtType::Struct: {
                Token name = deserialize_token(*this);
                const uint32_t field_count = read<uint32_t>();
                std::vector<StructField> fields;
                fields.reserve(field_count);
                for (uint32_t i = 0; i < field_count; ++i) {
                    fields.push_back(deserialize_struct_field(*this));
                }
                return std::make_unique<StructStmt>(name, std::move(fields));
            }
        }
        throw std::runtime_error("Malformed binary AST: Unknown statement type");
    }
} // namespace ObSL
