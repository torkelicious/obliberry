#ifndef OBLIBERRY_SCRIPTCOMPONENT_H
#define OBLIBERRY_SCRIPTCOMPONENT_H
#include <memory>
#include "Scripting/ObSLCore/Interpreter/Environment.h"
#include "Scripting/ObSLCore/Parser/Parser.h"

struct ScriptComponent {
    std::vector<std::string> scriptPaths;
    std::vector<std::shared_ptr<ObSL::Environment> > instance_envs;
    std::vector<ObSL::ObSLCallable *> on_update_functions;
    std::vector<ObSL::ObSLCallable *> on_destroy_functions;
    std::vector<ObSL::ObSLCallable *> on_exit_functions;
    std::vector<bool> isInitialized;
    std::vector<std::string> source_codes;
    std::vector<std::vector<std::unique_ptr<ObSL::Stmt> > > ast_nodes;
    std::vector<std::filesystem::file_time_type> lastModified;
};

#endif //OBLIBERRY_SCRIPTCOMPONENT_H
