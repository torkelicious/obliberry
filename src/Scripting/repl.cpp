#include "repl.h"
#include "Lexer/Lexer.h"
#include "Parser/Parser.h"
#include <iostream>
#include <memory>
#include <string>
#include <variant>

// temp for testing rn
namespace Scripting {
    struct ValuePrinter {
        void operator()(std::monostate) const { std::cout << "null"; }
        void operator()(const bool b) const { std::cout << (b ? "true" : "false"); }
        void operator()(const double d) const { std::cout << d; }
        void operator()(const std::string_view s) const { std::cout << '"' << s << '"'; }
    };

    // forward declarations for recursive printing
    void print_expr(const Expr *expr);

    void print_stmt(const Stmt *stmt);

    void print_expr(const Expr *expr) {
        if (!expr) return;
        if (const auto e = dynamic_cast<const LiteralExpr *>(expr)) {
            std::visit(ValuePrinter{}, e->value);
        } else if (const auto e = dynamic_cast<const BinaryExpr *>(expr)) {
            std::cout << "(" << e->oprt.lexeme << " ";
            print_expr(e->left.get());
            std::cout << " ";
            print_expr(e->right.get());
            std::cout << ")";
        } else if (const auto e = dynamic_cast<const GroupingExpr *>(expr)) {
            std::cout << "(group ";
            print_expr(e->expr.get());
            std::cout << ")";
        } else if (const auto e = dynamic_cast<const UnaryExpr *>(expr)) {
            std::cout << "(" << e->oprt.lexeme << " ";
            print_expr(e->right.get());
            std::cout << ")";
        } else if (const auto e = dynamic_cast<const VariableExpr *>(expr)) {
            std::cout << e->name.lexeme;
        } else if (const auto e = dynamic_cast<const AssignmentExpr *>(expr)) {
            std::cout << "(= " << e->name.lexeme << " ";
            print_expr(e->value.get());
            std::cout << ")";
        }
    }

    void print_stmt(const Stmt *stmt) {
        if (!stmt) return;

        if (const auto s = dynamic_cast<const ExpressionStmt *>(stmt)) {
            std::cout << "[ExprStmt: ";
            print_expr(s->expression.get());
            std::cout << "]\n";
        } else if (const auto s = dynamic_cast<const PrintStmt *>(stmt)) {
            std::cout << "[PrintStmt: ";
            print_expr(s->expression.get());
            std::cout << "]\n";
        } else if (const auto s = dynamic_cast<const BlockStmt *>(stmt)) {
            std::cout << "[BlockStmt: {\n";
            for (const auto &inner_stmt: s->statements) {
                std::cout << "  ";
                print_stmt(inner_stmt.get());
            }
            std::cout << "}]\n";
        }
    }

    void start_repl() {
        std::string line;
        std::cout << "obsl REPL\nType 'exit' to quit.\n";

        while (true) {
            std::cout << "> ";
            if (!std::getline(std::cin, line)) break;

            if (line == "exit") {
                std::cout << "Goodbye!\n";
                break;
            }

            Lexer lexer(line);
            const std::vector<Token> tokens = lexer.tokenize();

            Parser parser(tokens);
            try {
                for (auto statements = parser.parse(); const auto &stmt: statements) {
                    print_stmt(stmt.get());
                }
            } catch (const std::runtime_error &e) {
                std::cerr << "Parser Error: " << e.what() << "\n";
            }
        }
    }
}
