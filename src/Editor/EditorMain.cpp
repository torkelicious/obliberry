#include "../Core/Application.h"
#include "../Core/ProjectConfig.h"
#include "Core/Project.h"
#include "EditorLayer.h"
#include "Core/Logger.h"
#include "Core/LoggerService.h"

#include <filesystem>

int main(const int argc, char *argv[]) {
    Core::Logging::Logger<1000> logger;
    Core::Logging::LoggerService::Initialize(&logger);
    LOG_INFO("EditorMain", "Initialized");
    Core::ProjectConfig startupConfig; // default empty

    // CLI
    if (argc > 1) {
        if (const std::string projectPath = argv[1]; std::filesystem::exists(projectPath)) {
            Core::Project::Load(projectPath);
            startupConfig = Core::Project::GetActive()->GetConfig();
        }
    }

    // default
    Core::Application app(startupConfig, std::make_unique<Editor::EditorLayer>());
    app.Run();
    return 0;
}
