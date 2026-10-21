#pragma once

#include "nlohmann/json.hpp"
#include <ObSL/Environment.h>
#include <ObSL/Parser.h>
#include <filesystem>
#include <map>
#include <memory>
#include <string>

namespace ECS::Components {
    struct ScriptSlot {
        ScriptSlot() = default;
        ScriptSlot(ScriptSlot &&) noexcept = default;
        ScriptSlot &operator=(ScriptSlot &&) noexcept = default;
        ScriptSlot(const ScriptSlot &) = delete;
        ScriptSlot &operator=(const ScriptSlot &) = delete;

        bool isInitialized = false;
        std::string scriptPath;
        std::map<std::string, nlohmann::json> properties;
        std::filesystem::path resolvedPath;
        std::vector<std::shared_ptr<ObSL::Environment>> instance_envs;
        // hooks
        std::vector<ObSL::ObSLCallable *> on_update_functions;
        std::vector<ObSL::ObSLCallable *> on_destroy_functions;
        std::vector<ObSL::ObSLCallable *> on_exit_functions;
        // collider hooks
        std::vector<ObSL::ObSLCallable *> on_collision_enter_functions;
        std::vector<ObSL::ObSLCallable *> on_collision_stay_functions;
        std::vector<ObSL::ObSLCallable *> on_collision_exit_functions;
        // trigger collider hooks
        std::vector<ObSL::ObSLCallable *> on_trigger_enter_functions;
        std::vector<ObSL::ObSLCallable *> on_trigger_stay_functions;
        std::vector<ObSL::ObSLCallable *> on_trigger_exit_functions;
        // serialization / loading
        std::string source_code;
        std::vector<std::unique_ptr<ObSL::Stmt>> ast_nodes;
        std::filesystem::file_time_type lastModified;
    };

    struct ScriptComponent {
        std::vector<ScriptSlot> slots;
    };
} // namespace ECS::Components
