#pragma once

#include <unordered_map>
#include <string>
#include <ObSL/Interpreter.h>


namespace ECS::Components {
    // this is a generic container for custom Components defined in ObSL scripts.
    struct CustomDataComponent {
        using Root = std::shared_ptr<ObSL::Environment>;

        std::unordered_map<std::string, ObSL::Value> script_components;

        static Root ProtectValue(ObSL::Interpreter &interpreter, const ObSL::Value &value) {

            auto root = std::make_shared<ObSL::Environment>();
            root->define("value", value);
            interpreter.register_environment(root);
            return root;
        }

        void SetRooted(const std::string &name, Root root) {
            if (!root) {
                throw std::invalid_argument("Missing custom data GC root");
            }

            ObSL::Value value = root->get("value");
            auto [it, inserted] = m_Roots.try_emplace(name);

            try {
                script_components.insert_or_assign(name, std::move(value));
            } catch (...) {
                if (inserted) {
                    m_Roots.erase(it);
                }
                throw;
            }

            it->second = std::move(root);
        }

        void Set(const std::string &name, const ObSL::Value &value, ObSL::Interpreter &interpreter) { SetRooted(name, ProtectValue(interpreter, value)); }

        [[nodiscard]] ObSL::Value Get(const std::string &name) const {
            const auto it = m_Roots.find(name);
            if (it == m_Roots.end() || !it->second) {
                return std::monostate{};
            }

            const auto &values = it->second->get_values();
            const auto value = values.find("value");
            if (value == values.end()) {
                return std::monostate{};
            }

            return value->second;
        }

    private:
        std::unordered_map<std::string, Root> m_Roots;
    };

} // namespace ECS::Components
