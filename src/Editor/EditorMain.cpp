#include "EditorLayer.h"
#include "../Core/Application.h"
#include "../Core/ProjectConfig.h"
#include "../Game/GameLayer.h"

// TODO:
// this just runs game lol
// placeholder for cmake testing
int main(int argc, char *argv[]) {
    const ProjectConfig config = ProjectConfig::Deserialize("project.json");

    Application app(
    config,
    std::make_unique<EditorLayer>()
    );
    app.Run();
    return 0;
}

