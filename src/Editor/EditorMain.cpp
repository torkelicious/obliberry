#include "EditorLayer.h"
#include "../Core/Application.h"
#include "../Core/ProjectConfig.h"
#include "IO/VFS.h"

int main(int argc, char *argv[]) {
    std::string projectPath = "project.json";

    if (argc > 1) {
        projectPath = argv[1];
    }
    IO::VFS::MountProject(projectPath);
    const ProjectConfig config = ProjectConfig::Deserialize(IO::VFS::Resolve(projectPath).string());
    Application app(config, std::make_unique<EditorLayer>());
    app.Run();
    return 0;
}
