#pragma once

#include "nlohmann/json.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <map>
#include <regex>
#include <string>
#include <utility>
#include <vector>

namespace Scripting {

    using PropertyMap = std::map<std::string, nlohmann::json>;
    enum class ScriptPropertyType : uint8_t { Number, Bool, String, Entity, UIElement };

    struct ScriptPropertyDecl {
        std::string varName;
        std::string name;
        ScriptPropertyType type = ScriptPropertyType::Number;
        nlohmann::json defaultValue;
    };

    namespace detail {
        inline std::string Trim(const std::string &s) {
            const auto first = s.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) {
                return {};
            }
            const auto last = s.find_last_not_of(" \t\r\n");
            return s.substr(first, last - first + 1);
        }

        inline bool ParseLiteral(const std::string &raw, ScriptPropertyType &type, nlohmann::json &out) {
            const std::string text = Trim(raw);
            if (text.empty()) {
                return false;
            }
            if (text == "true" || text == "false") {
                type = ScriptPropertyType::Bool;
                out = (text == "true");
                return true;
            }
            if (text.size() >= 2 && text.front() == '"' && text.back() == '"') {
                type = ScriptPropertyType::String;
                out = text.substr(1, text.size() - 2);
                return true;
            }

            char *end = nullptr;
            const double number = std::strtod(text.c_str(), &end);
            if (end != text.c_str() && *end == '\0') {
                type = ScriptPropertyType::Number;
                out = number;
                return true;
            }
            return false;
        }
    } // namespace detail

    inline std::vector<ScriptPropertyDecl> ScanScriptProperties(const std::string &src) {
        // i hate this
        static const std::regex pattern(R"re(^\s*var\s+([A-Za-z_]\w*)\s*=\s*(Prop|PropEntity|PropUI)\s*\(\s*"([^"]+)"\s*(?:,\s*(.+?))?\s*\)\s*;)re");

        std::vector<ScriptPropertyDecl> result;
        std::istringstream stream(src);
        std::string line;

        while (std::getline(stream, line)) {
            if (line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            std::smatch m;
            if (!std::regex_search(line, m, pattern)) {
                continue;
            }

            ScriptPropertyDecl decl;
            decl.varName = m[1].str();
            decl.name = m[3].str();

            if (m[2].str() == "PropEntity") {
                decl.type = ScriptPropertyType::Entity;
            } else if (m[2].str() == "PropUI") {
                decl.type = ScriptPropertyType::UIElement;
            } else if (!m[4].matched || !detail::ParseLiteral(m[4].str(), decl.type, decl.defaultValue)) {
                continue;
            }

            bool duplicate = false;
            for (const auto &existing : result) {
                if (existing.name == decl.name) {
                    duplicate = true;
                    break;
                }
            }

            if (!duplicate) {
                result.push_back(std::move(decl));
            }
        }
        return result;
    }

    inline std::atomic<const PropertyMap *> g_InitProperties{nullptr};

    inline const PropertyMap *CurrentInitProps() { return g_InitProperties.load(std::memory_order::acquire); }

    struct ScriptPropertyScope {
        explicit ScriptPropertyScope(const PropertyMap *p) { g_InitProperties.store(p, std::memory_order_release); }
        ~ScriptPropertyScope() { g_InitProperties.store(nullptr, std::memory_order_release); }
        ScriptPropertyScope(const ScriptPropertyScope &) = delete;
        ScriptPropertyScope &operator=(const ScriptPropertyScope &) = delete;
    };


} // namespace Scripting
