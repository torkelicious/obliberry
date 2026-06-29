#include "../Core/Application.h"
#include "../Core/ProjectConfig.h"
#include "Core/Project.h"
#include "EditorLayer.h"
#include "IO/VFS.h"
#include <filesystem>

int main(const int argc, char *argv[]) {
    Core::ProjectConfig startupConfig; // default empty

    // CLI
    if (argc > 1) {
        const std::string projectPath = argv[1];
        if (std::filesystem::exists(projectPath)) {
            Core::Project::Load(projectPath);
            startupConfig = Core::Project::GetActive()->GetConfig();
        }
    }

    // default
    Core::Application app(startupConfig, std::make_unique<Editor::EditorLayer>());
    app.Run();
    return 0;
}
