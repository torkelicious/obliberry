#pragma once
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <ObSL/Parser/ast.h>

namespace ObSL {
    struct SerializedModule {
        std::vector<std::string> string_pool;
        std::vector<std::unique_ptr<Stmt> > statements;
    };


    class ASTSerializer {
    public:
        std::vector<uint8_t> buffer;
        std::unordered_map<std::string_view, uint32_t> string_to_index;
        std::vector<std::string_view> string_table;

        uint32_t get_string_index(std::string_view str) {
            if (const auto it = string_to_index.find(str); it != string_to_index.end()) {
                return it->second;
            }
            const auto idx = static_cast<uint32_t>(string_table.size());
            string_table.push_back(str);
            string_to_index[str] = idx;
            return idx;
        }

        template<typename T>
        void write(const T &val) {
            const auto *ptr = reinterpret_cast<const uint8_t *>(&val);
            buffer.insert(buffer.end(), ptr, ptr + sizeof(T));
        }

        void write_string_index(const std::string_view str) {
            write<uint32_t>(get_string_index(str));
        }

        void serialize_expr(const Expr *expr);

        void serialize_stmt(const Stmt *stmt);

        std::vector<uint8_t> finalize(const std::vector<std::unique_ptr<Stmt> > &root_ast) {
            // build the node payload block first
            for (auto &stmt: root_ast) {
                serialize_stmt(stmt.get());
            }
            std::vector<uint8_t> node_payload = std::move(buffer);
            buffer.clear();

            // Structure layout layout:
            // [String Table Size] [String 1 Size][Chars...] ... [Statements Count] [Nodes Payload]
            write<uint32_t>(static_cast<uint32_t>(string_table.size()));
            for (auto str: string_table) {
                write<uint32_t>(static_cast<uint32_t>(str.size()));
                buffer.insert(buffer.end(), str.begin(), str.end());
            }

            write<uint32_t>(static_cast<uint32_t>(root_ast.size()));
            buffer.insert(buffer.end(), node_payload.begin(), node_payload.end());

            return std::move(buffer);
        }
    };
} // ObSL
