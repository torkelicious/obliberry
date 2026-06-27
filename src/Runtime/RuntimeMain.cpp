#include "../Core/Application.h"
#include "../Core/ProjectConfig.h"
#include "../Game/GameLayer.h"

int main(int argc, char *argv[]) {
    const ProjectConfig config = ProjectConfig::Deserialize("project.json");

    Application app(
        config,
        std::make_unique<GameLayer>()
    );
    app.Run();
    return 0;
}
