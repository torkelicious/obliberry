#pragma once
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <ObSL/Parser/ast.h>

#include "ASTSerializer.h"

namespace ObSL {
    class ASTDeserializer {
    private:
        const uint8_t *m_ptr;
        const uint8_t *m_end;
        const std::vector<std::string> &m_pool;

    public:
        template<typename T>
        T read() {
            if (m_ptr + sizeof(T) > m_end) {
                throw std::runtime_error("Malformed binary AST: Out of bounds read");
            }
            T val;
            std::memcpy(&val, m_ptr, sizeof(T));
            m_ptr += sizeof(T);
            return val;
        }

        std::string_view read_string_view() {
            uint32_t idx = read<uint32_t>();
            if (idx >= m_pool.size()) {
                throw std::runtime_error("Malformed binary AST: Invalid string table index");
            }
            return std::string_view(m_pool[idx]);
        }

        ASTDeserializer(const std::vector<uint8_t> &data, const std::vector<std::string> &pool)
            : m_ptr(data.data()), m_end(data.data() + data.size()), m_pool(pool) {
        }

        std::unique_ptr<Expr> deserialize_expr();


        std::unique_ptr<Stmt> deserialize_stmt();

        static SerializedModule deserialize(const std::vector<uint8_t> &data) {
            if (data.size() < sizeof(uint32_t)) {
                throw std::runtime_error("Malformed data block");
            }

            const uint8_t *ptr = data.data();
            const uint8_t *end = data.data() + data.size();

            // reconstruct String Table Pool
            uint32_t string_count;
            std::memcpy(&string_count, ptr, sizeof(uint32_t));
            ptr += sizeof(uint32_t);

            std::vector<std::string> pool;
            pool.reserve(string_count);

            for (uint32_t i = 0; i < string_count; ++i) {
                uint32_t len;
                std::memcpy(&len, ptr, sizeof(uint32_t));
                ptr += sizeof(uint32_t);

                std::string str(reinterpret_cast<const char *>(ptr), len);
                ptr += len;
                pool.push_back(std::move(str));
            }

            // root Statement Count
            uint32_t stmt_count;
            std::memcpy(&stmt_count, ptr, sizeof(uint32_t));
            ptr += sizeof(uint32_t);

            // deserialize nodes using layout cursor
            const std::vector<uint8_t> node_payload(ptr, end);
            ASTDeserializer deserializer(node_payload, pool);

            SerializedModule module;
            module.statements.reserve(stmt_count);

            for (uint32_t i = 0; i < stmt_count; ++i) {
                module.statements.push_back(deserializer.deserialize_stmt());
            }
            module.string_pool = std::move(pool);

            return module;
        }
    };
} // namespace ObSL
