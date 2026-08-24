#pragma once

#include <ObSL/Environment.h>
#include <ObSL/Parser.h>
#include <filesystem>
#include <memory>

namespace ECS::Components {
    struct ScriptSlot {
        bool isInitialized = false;
        std::string scriptPath;
        std::filesystem::path resolvedPath;
        std::vector<std::shared_ptr<ObSL::Environment>> instance_envs;
        std::vector<ObSL::ObSLCallable *> on_update_functions;
        std::vector<ObSL::ObSLCallable *> on_destroy_functions;
        std::vector<ObSL::ObSLCallable *> on_exit_functions;
        std::string source_code;
        std::vector<std::unique_ptr<ObSL::Stmt>> ast_nodes;
        std::filesystem::file_time_type lastModified;
    };

    struct ScriptComponent {
        std::vector<ScriptSlot> slots;
    };
} // namespace ECS::Components
