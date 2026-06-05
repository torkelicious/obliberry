#include <chrono>
#include "Graphics/GLDebug.h"
#include "ECS/Systems/InputSystem.h"
#include "Application.h"
#include "Window.h"
#include "Graphics/Renderer.h"

Application::Application()
    : m_Window(1280, 720, "Window", true, this),
      m_GameWorld(m_InputManager) {
    m_Window.SetInputManager(&m_InputManager);
}

void Application::Run() {
    GLDebug::InitDebug();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Renderer renderer;
    m_Window.SetCamera(&m_GameWorld.GetCamera());

    InputSystem inputSystem(m_InputManager);
    auto previousTime = std::chrono::steady_clock::now();

    while (!m_Window.ShouldClose()) {
        m_InputManager.BeginFrame();
        m_Window.PollEvents();

        const auto currentTime = std::chrono::steady_clock::now();
        const std::chrono::duration<float> delta = currentTime - previousTime;
        previousTime = currentTime;

        inputSystem.Update(m_GameWorld.GetPlayer());
        m_GameWorld.Update(delta.count());

        m_GameWorld.Render(renderer);

        m_Window.SwapBuffers();
        m_Window.PollEvents();
    }
}

void Application::Shutdown() {
    m_GameWorld.Shutdown();
    m_Window.Shutdown();
}
