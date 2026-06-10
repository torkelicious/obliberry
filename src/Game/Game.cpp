#include "Game.h"
#include "IO/AssetLoader.h"
#include "IO/MapSerialization.h"
#include "Renderer/MeshFactory.h"
#include <filesystem>


bool Game::TestFileWrite(const HexGrid &grid, const std::string &path) const {
    const size_t expectedBytes = MapIO::CalculateExpectedFileSize(grid.tiles.size());
    if (!MapIO::Serialize(path, grid)) return false;
    return std::filesystem::file_size(path) == expectedBytes;
}

bool Game::TestFileLoad(HexGrid &grid, const std::string &path) const {
    return MapIO::Deserialize(path, grid);
}

void Game::Start() {
    AssetLoader::RegisterMeshFactory("Quad", []() -> std::shared_ptr<Mesh> {
        auto data = MeshFactory::CreateQuad();
        return std::make_shared<Mesh>(data.vertices, data.indices);
    });
    AssetLoader::RegisterMeshFactory("PointTopHex", []() -> std::shared_ptr<Mesh> {
        auto data = MeshFactory::CreatePointTopHex(0.5f);
        return std::make_shared<Mesh>(data.vertices, data.indices);
    });

    m_SceneManager.LoadScene(
        std::make_unique<Scene>(m_Context, "assets/scenes/level01.json"));
}

void Game::Update(float dt) {
    m_Context.deltaTime = dt;

    if (m_PendingSceneLoad.has_value()) {
        m_SceneManager.LoadScene(
            std::make_unique<Scene>(m_Context, std::move(*m_PendingSceneLoad)));
        m_PendingSceneLoad.reset();
        return;
    }

    DrawInterface();
    if (m_CurrentState == GameState::Gameplay) {
        m_SceneManager.Update(dt);
    }
}

void Game::Render(Renderer &renderer) {
    if (m_Context.camera) {
        renderer.SetCamera(*m_Context.camera);
    }
    renderer.BeginFrame();
    m_SceneManager.Render(renderer);
    m_Context.lastDrawCallCount = renderer.GetLastDrawCallCount();
}

void Game::Shutdown() const {
}
