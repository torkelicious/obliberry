#ifndef OBLIBERRY_SCRIPTCOMPONENT_H
#define OBLIBERRY_SCRIPTCOMPONENT_H
#include <memory>
#include "Core/Constants.h"
#include "Core/Utils.h"
#include "Scripting/ObSLCore/Interpreter/Environment.h"
#include "Scripting/ObSLCore/Parser/Parser.h"

struct ScriptComponent {
    std::string scriptPath = PathUtils::Join(SCRIPT_PATH, "test", SCRIPT_FILE_EXTENSION);
    std::shared_ptr<ObSL::Environment> instance_env = nullptr;
    ObSL::ObSLCallable *on_update = nullptr;
    ObSL::ObSLCallable *on_destroy = nullptr;
    ObSL::ObSLCallable *on_exit = nullptr;
    bool isInitialized = false;
    std::string source_code;
    std::vector<std::unique_ptr<ObSL::Stmt> > ast_nodes;
    std::filesystem::file_time_type lastModified;
};

#endif //OBLIBERRY_SCRIPTCOMPONENT_H
