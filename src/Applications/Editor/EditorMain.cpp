#include "Core/Application.h"
#include "Config/ProjectConfig.h"
#include "Config/GraphicsConfig.h"
#include "Core/Project.h"
#include "EditorLayer.h"
#include "Logger/Logger.h"
#include "Logger/LoggerService.h"
#include <filesystem>

int main(const int argc, char *argv[]) {
    Platform::Window::Window::s_ShouldInitNFD = true;
    Logging::Logger<1000> logger;
    Logging::LoggerService::Initialize(&logger);
    LOG_INFO("EditorMain", "Initialized");

    // default empty
    Config::ProjectConfig projectConfig;
    Config::GraphicsConfig graphicsConfig;

    Editor::EditorLayer::s_ShouldBuildDock = !(std::filesystem::exists("imgui.ini"));
    // CLI
    if (argc > 1) {
        if (const std::string projectPath = argv[1]; std::filesystem::exists(projectPath)) {
            Core::Project::Load(projectPath);
            projectConfig = Core::Project::GetActive()->GetConfig();
            graphicsConfig = Config::GraphicsConfig::Deserialize("graphics.json");
        }
    }

    // default
    Core::Application app(graphicsConfig, projectConfig, std::make_unique<Editor::EditorLayer>());
    app.Run();
    return 0;
}
