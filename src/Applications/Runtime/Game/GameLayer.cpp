#include "GameLayer.h"
#include "IO/Loaders/AssetLoader.h"
#include "IO/VFS/VFS.h"
#include <filesystem>
#include <ObSL/ScriptRuntime.h>
#include "Config/ProjectConfig.h"
#include "Platform/Window/Window.h"
#include "Rendering/Renderer.h"
#include "Scripting/EngineLib/EngineLib.h"
#include "UI/Text/Font.h"
#include "UI/Rendering/UIRenderer.h"

// temp
//static std::shared_ptr<UI::Font> s_TestFont;

void Game::GameLayer::Init(Core::EngineContext &ctx) {
    m_Context = &ctx;

    // temp
    //s_TestFont = std::make_shared<UI::Font>("/usr/share/fonts/TTF/DejaVuSans.ttf", 32);
    //s_TestFont->InitGL();

    m_Context->sceneManager = &m_SceneManager;

    m_SceneManager.SetContext(*m_Context);

    if (m_Context->scriptPool) {
        m_Context->scriptPool->init(IO::VFS::GetAssetsDirectory() / "scripts");
    }

    std::string startScene;
    if (m_Context->projectConfig) {
        startScene = m_Context->projectConfig->startScenePath;
    }

    if (!startScene.empty()) {
        m_SceneManager.LoadSceneByPath(startScene);
    }
}

void Game::GameLayer::Update(const float dt) {
    if (!m_Context)
        return;

    m_Context->deltaTime = dt;

    m_SceneManager.ProcessPendingSceneChange(*m_Context);

    DrawInterface();
    if (m_GameIsRunning) {
        m_SceneManager.Update(dt);
    }
}

void Game::GameLayer::Render() {
    if (!m_Context)
        return;
    if (m_Context->camera && m_Context->window) {
        const float aspect = static_cast<float>(m_Context->window->GetWidth()) / static_cast<float>(m_Context->window->GetHeight());
        m_Context->renderer->SetCamera(*m_Context->camera, aspect);
    }
    m_SceneManager.Render();

    // ui system runtime-tests
    //// temp
    //m_Context->uiRenderer->BeginFrame(m_Context->window->GetWidth(), m_Context->window->GetHeight());
    //
    //// temp render text quads
    //if (s_TestFont && s_TestFont->IsValid()) {
    //    const std::string text = "Hello World..!";
    //    float cursorX = 100.0f;
    //    const float baselineY = 100.0f;
    //
    //    for (const char c : text) {
    //        const auto &glyph = s_TestFont->GetGlyph(c);
    //        if (glyph.Size.x > 0 && glyph.Size.y > 0) {
    //            const float x = cursorX + static_cast<float>(glyph.Bearing.x);
    //            const float y = baselineY - static_cast<float>(glyph.Bearing.y);
    //            const auto w = static_cast<float>(glyph.Size.x);
    //            const auto h = static_cast<float>(glyph.Size.y);
    //
    //            m_Context->uiRenderer->SubmitQuad({x, y}, {w, h}, glyph.UVOffset, glyph.UVOffset + glyph.UVSize, s_TestFont->GetAtlasTexture().get(), {0.5, 0.5, 0, 1});
    //        }
    //        cursorX += static_cast<float>(glyph.Advance);
    //    }
    //}
    //// untextured rect test
    //m_Context->uiRenderer->SubmitRect({100, 200}, {200, 100}, {0.5, 0, 0.5, 1});
    //// temp
}

void Game::GameLayer::Shutdown() {}
