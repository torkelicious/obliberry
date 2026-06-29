#include "../Core/Application.h"
#include "../Core/ProjectConfig.h"
#include "../Game/GameLayer.h"

int main(int argc, char *argv[]) {
    const Core::ProjectConfig config = Core::ProjectConfig::Deserialize("project.json");

    Core::Application app(
        config,
        std::make_unique<Game::GameLayer>()
    );
    app.Run();
    return 0;
}
