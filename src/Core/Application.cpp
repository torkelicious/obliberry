#include "Application.h"
#include "Graphics/Renderer.h"
#include "Graphics/GLDebug.h"

Application::Application()
    : m_Window(1280, 720, "Window", true),
      m_GameWorld() {
}

void Application::Run() {
    GLDebug::initDbg();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Renderer renderer;
    m_Window.SetCamera(&m_GameWorld.GetCamera());

    while (!m_Window.ShouldClose()) {
        m_GameWorld.Render(renderer);

        m_Window.SwapBuffers();
        m_Window.PollEvents();
    }
}
