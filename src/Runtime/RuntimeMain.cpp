#include "../Core/Application.h"
#include "../Core/ProjectConfig.h"
#include "../Core/GraphicsConfig.h"
#include "../Game/GameLayer.h"
#include "Core/Logger.h"
#include "Core/LoggerService.h"
#include "IO/VFS.h"
#include <filesystem>

int main(const int argc, char *argv[]) {
    Core::Logging::Logger<1000> logger;
    Core::Logging::LoggerService::Initialize(&logger);
    std::filesystem::path targetPackage = "data.obpak";
    std::filesystem::path targetProject = "project.json";
    bool mountSuccess = false;

    for (int i = 1; i < argc; ++i) {
        if (std::string arg = argv[i]; (arg == "-p" || arg == "--project") && i + 1 < argc) {
            targetProject = argv[++i];
            IO::VFS::MountProject(targetProject);
            mountSuccess = true;
            break;
        } else if ((arg == "-pk" || arg == "--package") && i + 1 < argc) {
            targetPackage = argv[++i];
            IO::VFS::MountPackage(targetPackage);
            mountSuccess = true;
            break;
        } else if (std::filesystem::path(arg).extension() == ".obpak") {
            targetPackage = arg;
            IO::VFS::MountPackage(targetPackage);
            mountSuccess = true;
            break;
        }
    }

    if (!mountSuccess) {
        if (std::filesystem::exists(targetPackage)) {
            LOG_INFO("Runtime", "Autodetected packaged archive: " + targetPackage.string());
            IO::VFS::MountPackage(targetPackage);
        } else if (std::filesystem::exists(targetProject)) {
            LOG_INFO("Runtime", "Autodetected project workspace: " + targetProject.string());
            IO::VFS::MountProject(targetProject);
        } else {
            LOG_WARN("Runtime", "No execution context found. Operating with an empty VFS");
        }
    }

    const Core::ProjectConfig config = Core::ProjectConfig::Deserialize("project.json");
    const Core::Graphics::GraphicsConfig graphicsConfig = Core::Graphics::GraphicsConfig::Deserialize("graphics.json");

    Core::Application app(graphicsConfig, config, std::make_unique<Game::GameLayer>());
    app.Run();
    return 0;
}
