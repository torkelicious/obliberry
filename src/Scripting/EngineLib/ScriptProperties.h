#pragma once

#include "nlohmann/json.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
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
        bool hasDefaultValue = false;
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
                try {
                    if (const auto parsed = nlohmann::json::parse(text); parsed.is_string()) {
                        out = parsed;
                        return true;
                    }
                } catch (const nlohmann::json::exception &) {
                }
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

        inline ScriptPropertyType InferExpressionType(const std::string &raw) {
            const std::string text = Trim(raw);
            if (!text.empty() && text.front() == '"') {
                return ScriptPropertyType::String;
            }
            if (text == "true" || text == "false") {
                return ScriptPropertyType::Bool;
            }
            return ScriptPropertyType::Number;
        }
    } // namespace detail

    inline std::vector<ScriptPropertyDecl> ScanScriptProperties(const std::string &src) {
        // i hate this
        static const std::regex pattern(R"re(^var\s+([A-Za-z_]\w*)\s*=\s*(Prop|PropEntity|PropUI)\s*\(\s*"([^"]+)"\s*(?:,\s*(.+?))?\s*\)\s*;\s*(?://.*)?$)re");
        static const std::regex startPattern(R"re(^var\s+[A-Za-z_]\w*\s*=\s*(?:Prop|PropEntity|PropUI)\s*\()re");
        static const std::regex completePattern(R"re(;\s*(?://.*)?$)re");

        std::vector<ScriptPropertyDecl> result;
        std::istringstream stream(src);
        std::string line;
        std::string statement;

        const auto addDecl = [&result](const std::smatch &m) {
            ScriptPropertyDecl decl;
            decl.varName = m[1].str();
            decl.name = m[3].str();

            if (m[2].str() == "PropEntity") {
                decl.type = ScriptPropertyType::Entity;
            } else if (m[2].str() == "PropUI") {
                decl.type = ScriptPropertyType::UIElement;
            } else if (!m[4].matched) {
                return;
            } else if (detail::ParseLiteral(m[4].str(), decl.type, decl.defaultValue)) {
                decl.hasDefaultValue = true;
            } else {
                decl.type = detail::InferExpressionType(m[4].str());
            }

            for (const auto &existing : result) {
                if (existing.name == decl.name) {
                    return;
                }
            }
            result.push_back(std::move(decl));
        };

        while (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            const std::string trimmed = detail::Trim(line);

            if (statement.empty()) {
                if (!std::regex_search(trimmed, startPattern)) {
                    continue;
                }
                statement = trimmed;
            } else {
                if (std::regex_search(trimmed, startPattern)) {
                    statement = trimmed;
                } else if (!trimmed.empty()) {
                    statement += " ";
                    statement += trimmed;
                }
            }

            if (!std::regex_search(statement, completePattern)) {
                continue;
            }

            std::smatch m;
            if (std::regex_match(statement, m, pattern)) {
                addDecl(m);
            }
            statement.clear();
        }
        return result;
    }

    inline thread_local const PropertyMap *g_InitProperties = nullptr;

    inline const PropertyMap *CurrentInitProps() { return g_InitProperties; }

    struct ScriptPropertyScope {
        explicit ScriptPropertyScope(const PropertyMap *p) : m_Previous(g_InitProperties) { g_InitProperties = p; }
        ~ScriptPropertyScope() { g_InitProperties = m_Previous; }
        ScriptPropertyScope(const ScriptPropertyScope &) = delete;
        ScriptPropertyScope &operator=(const ScriptPropertyScope &) = delete;

    private:
        const PropertyMap *m_Previous;
    };


} // namespace Scripting
