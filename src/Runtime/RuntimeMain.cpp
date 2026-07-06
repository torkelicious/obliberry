#include "../Core/Application.h"
#include "../Core/ProjectConfig.h"
#include "../Game/GameLayer.h"
#include "IO/VFS.h"
#include <filesystem>
#include <iostream>

int main(const int argc, char *argv[]) {
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
            std::cout << "[Runtime] Autodetected packaged archive: " << targetPackage.string() << "\n";
            IO::VFS::MountPackage(targetPackage);
        } else if (std::filesystem::exists(targetProject)) {
            std::cout << "[Runtime] Autodetected project workspace: " << targetProject.string() << "\n";
            IO::VFS::MountProject(targetProject);
        } else {
            std::cerr << "[Runtime] Warning: No execution context found. Operating with an empty VFS.\n";
        }
    }

    const Core::ProjectConfig config = Core::ProjectConfig::Deserialize("project.json");

    Core::Application app(config, std::make_unique<Game::GameLayer>());
    app.Run();
    return 0;
}
