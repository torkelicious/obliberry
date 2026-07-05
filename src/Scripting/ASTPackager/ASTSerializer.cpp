#include "ASTSerializer.h"

namespace ObSL {
    // helper utilities

    static void serialize_token(ASTSerializer &ser, const Token &token) {
        ser.write(token.type); // TokenType (uint8_t)
        ser.write_string_index(token.lexeme);
        ser.write(token.line); // uint16_t
        ser.write(token.column); // uint16_t
        ser.write(token.start_pos); // uint32_t
        ser.write(token.end_pos); // uint32_t
    }

    static void serialize_value(ASTSerializer &ser, const Value &value) {
        uint8_t variant_idx = static_cast<uint8_t>(value.index());

        // Runtime pointer variants (ObSLCallable*, ObSLArray*, ObSLObject*)
        // cannot be persisted serialize them as monostate instead.
        if (variant_idx >= 4) {
            variant_idx = 0;
        }

        ser.write(variant_idx);

        switch (variant_idx) {
            case 0: // monostate
                break;
            case 1: // bool
                ser.write(static_cast<uint8_t>(std::get<bool>(value)));
                break;
            case 2: // double
                ser.write(std::get<double>(value));
                break;
            case 3: // string
                ser.write_string_index(std::get<std::string>(value));
                break;
        }
    }

    static void serialize_param(ASTSerializer &ser, const Param &param) {
        ser.write_string_index(param.name);
        ser.serialize_expr(param.default_value.get());
    }

    static void serialize_struct_field(ASTSerializer &ser, const StructField &field) {
        serialize_token(ser, field.name);
        ser.serialize_expr(field.default_value.get());
    }

    static void serialize_case_branch(ASTSerializer &ser, const CaseBranch &branch) {
        ser.serialize_expr(branch.match_value.get());
        ser.write<uint32_t>(static_cast<uint32_t>(branch.statements.size()));
        for (const auto &stmt: branch.statements) {
            ser.serialize_stmt(stmt.get());
        }
    }

    // expression serialization

    void ASTSerializer::serialize_expr(const Expr *expr) {
        if (!expr) {
            // 0xFF for null pointer
            write<uint8_t>(0xFF);
            return;
        }

        const ExprType etype = expr->type();
        write(etype);

        switch (etype) {
            case ExprType::Call: {
                const auto *node = static_cast<const CallExpr *>(expr);
                serialize_expr(node->callee.get());
                serialize_token(*this, node->paren);
                write<uint32_t>(static_cast<uint32_t>(node->arguments.size()));
                for (const auto &arg: node->arguments) {
                    serialize_expr(arg.get());
                }
                break;
            }
            case ExprType::Literal: {
                const auto *node = static_cast<const LiteralExpr *>(expr);
                serialize_token(*this, node->token);
                serialize_value(*this, node->value);
                break;
            }
            case ExprType::Binary: {
                const auto *node = static_cast<const BinaryExpr *>(expr);
                serialize_expr(node->left.get());
                write(node->oprt_type);
                serialize_expr(node->right.get());
                break;
            }
            case ExprType::Logical: {
                const auto *node = static_cast<const LogicalExpr *>(expr);
                serialize_expr(node->left.get());
                write(node->oprt_type);
                serialize_expr(node->right.get());
                break;
            }
            case ExprType::Grouping: {
                const auto *node = static_cast<const GroupingExpr *>(expr);
                serialize_expr(node->expr.get());
                break;
            }
            case ExprType::Unary: {
                const auto *node = static_cast<const UnaryExpr *>(expr);
                write(node->oprt_type);
                serialize_expr(node->right.get());
                break;
            }
            case ExprType::Variable: {
                const auto *node = static_cast<const VariableExpr *>(expr);
                write_string_index(node->name);
                break;
            }
            case ExprType::Assignment: {
                const auto *node = static_cast<const AssignmentExpr *>(expr);
                write_string_index(node->name);
                serialize_expr(node->value.get());
                break;
            }
            case ExprType::Update: {
                const auto *node = static_cast<const UpdateExpr *>(expr);
                write_string_index(node->name);
                write(node->oprt_type);
                write(static_cast<uint8_t>(node->is_prefix)); // bitfield cast to byte
                break;
            }
            case ExprType::Array: {
                const auto *node = static_cast<const ArrayExpr *>(expr);
                write<uint32_t>(static_cast<uint32_t>(node->elements.size()));
                for (const auto &elem: node->elements) {
                    serialize_expr(elem.get());
                }
                break;
            }
            case ExprType::Index: {
                const auto *node = static_cast<const IndexExpr *>(expr);
                serialize_expr(node->callee.get());
                serialize_token(*this, node->bracket);
                serialize_expr(node->index.get());
                break;
            }
            case ExprType::IndexAssignment: {
                const auto *node = static_cast<const IndexAssignmentExpr *>(expr);
                serialize_expr(node->callee.get());
                serialize_token(*this, node->bracket);
                serialize_expr(node->index.get());
                serialize_expr(node->value.get());
                break;
            }
            case ExprType::Get: {
                const auto *node = static_cast<const GetExpr *>(expr);
                serialize_expr(node->obj.get());
                write_string_index(node->name);
                break;
            }
            case ExprType::Set: {
                const auto *node = static_cast<const SetExpr *>(expr);
                serialize_expr(node->obj.get());
                write_string_index(node->name);
                serialize_expr(node->value.get());
                break;
            }
            case ExprType::TypeCheck: {
                const auto *node = static_cast<const TypeCheckExpr *>(expr);
                serialize_expr(node->left.get());
                write_string_index(node->type_name);
                break;
            }
        }
    }

    // statement serialization

    void ASTSerializer::serialize_stmt(const Stmt *stmt) {
        if (!stmt) {
            write<uint8_t>(0xFF);
            return;
        }

        const StmtType stype = stmt->type();
        write(stype);

        switch (stype) {
            case StmtType::Using: {
                const auto *node = static_cast<const UsingStmt *>(stmt);
                write_string_index(node->path);
                break;
            }
            case StmtType::Expression: {
                const auto *node = static_cast<const ExpressionStmt *>(stmt);
                serialize_expr(node->expression.get());
                break;
            }
            case StmtType::Print: {
                const auto *node = static_cast<const PrintStmt *>(stmt);
                serialize_expr(node->expression.get());
                break;
            }
            case StmtType::Println: {
                const auto *node = static_cast<const PrintlnStmt *>(stmt);
                serialize_expr(node->expression.get());
                break;
            }
            case StmtType::Block: {
                const auto *node = static_cast<const BlockStmt *>(stmt);
                write<uint32_t>(static_cast<uint32_t>(node->statements.size()));
                for (const auto &s: node->statements) {
                    serialize_stmt(s.get());
                }
                break;
            }
            case StmtType::Function: {
                const auto *node = static_cast<const FunctionStmt *>(stmt);
                write_string_index(node->name);
                write<uint32_t>(static_cast<uint32_t>(node->params.size()));
                for (const auto &param: node->params) {
                    serialize_param(*this, param);
                }
                serialize_stmt(node->body.get());
                break;
            }
            case StmtType::If: {
                const auto *node = static_cast<const IfStmt *>(stmt);
                serialize_expr(node->condition.get());
                serialize_stmt(node->then_branch.get());
                serialize_stmt(node->else_branch.get());
                break;
            }
            case StmtType::Switch: {
                const auto *node = static_cast<const SwitchStmt *>(stmt);
                serialize_expr(node->condition.get());
                write<uint32_t>(static_cast<uint32_t>(node->cases.size()));
                for (const auto &c: node->cases) {
                    serialize_case_branch(*this, c);
                }
                break;
            }
            case StmtType::While: {
                const auto *node = static_cast<const WhileStmt *>(stmt);
                serialize_expr(node->condition.get());
                serialize_stmt(node->body.get());
                break;
            }
            case StmtType::Foreach: {
                const auto *node = static_cast<const ForeachStmt *>(stmt);
                write_string_index(node->loop_var);
                serialize_expr(node->iterable.get());
                serialize_stmt(node->body.get());
                break;
            }
            case StmtType::Return: {
                const auto *node = static_cast<const ReturnStmt *>(stmt);
                serialize_expr(node->value.get());
                break;
            }
            case StmtType::Break: {
                // no additional data
                break;
            }
            case StmtType::Var: {
                const auto *node = static_cast<const VarStmt *>(stmt);
                write_string_index(node->name);
                serialize_expr(node->initializer.get());
                break;
            }
            case StmtType::TryCatch: {
                const auto *node = static_cast<const TryCatchStmt *>(stmt);
                serialize_stmt(node->try_body.get());
                write_string_index(node->exception_var);
                serialize_stmt(node->catch_body.get());
                break;
            }
            case StmtType::Struct: {
                const auto *node = static_cast<const StructStmt *>(stmt);
                serialize_token(*this, node->name);
                write<uint32_t>(static_cast<uint32_t>(node->fields.size()));
                for (const auto &field: node->fields) {
                    serialize_struct_field(*this, field);
                }
                break;
            }
        }
    }
} // namespace ObSL
