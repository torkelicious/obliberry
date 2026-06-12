#include "Game.h"
#include "IO/AssetLoader.h"
#include "IO/MapSerialization.h"
#include "Renderer/MeshFactory.h"
#include <filesystem>

#include "Renderer/Renderer.h"


bool Game::TestFileWrite(const HexGrid &grid, const std::string &path) {
    const size_t expectedBytes = MapIO::CalculateExpectedFileSize(grid.tiles.size());
    if (!MapIO::Serialize(path, grid)) return false;
    return std::filesystem::file_size(path) == expectedBytes;
}

bool Game::TestFileLoad(HexGrid &grid, const std::string &path) {
    return MapIO::Deserialize(path, grid);
}

void Game::Start() {
    MeshFactory::RegisterAllMeshFactories();
    m_SceneManager.LoadScene(
        std::make_unique<Scene>
        (m_Context,
         SceneProperties{.ScenePath = "assets/scenes/test1.json"}
        ));
}

void Game::Update(const float dt) {
    m_Context.deltaTime = dt;

    if (m_PendingSceneLoad.has_value()) {
        m_SceneManager.LoadScene(
            std::make_unique<Scene>(m_Context, std::move(*m_PendingSceneLoad)));
        m_PendingSceneLoad.reset();
    }

    DrawInterface();
    if (m_CurrentState == GameState::Gameplay) {
        m_SceneManager.Update(dt);
    }
}

void Game::Render() const {
    if (m_Context.camera) {
        m_Context.renderer->SetCamera(*m_Context.camera);
    }
    m_Context.renderer->BeginFrame();
    m_SceneManager.Render();
}

void Game::Shutdown() const {
}

