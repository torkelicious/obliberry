#include "Application.h"
#include <chrono>
#include <thread>
#include "Constants.h"
#include "Game/Game.h"
#include "Renderer/Renderer.h"
#include "Core/EngineContext.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

Application::Application()
    : m_Window(WINDOW_WIDTH, WINDOW_HEIGHT, "obliberry") {
    m_Window.SetInputManager(&m_InputManager);
}

void Application::Run() {
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(m_Window.GetNativeWindow(), true);
    ImGui_ImplOpenGL3_Init();
    ImGui::StyleColorsDark();

    Renderer renderer;
    Camera camera;

    EngineContext context;
    context.window = &m_Window;
    context.input = &m_InputManager;
    context.resources = &m_ResourceManager;
    context.camera = &camera;
    context.deltaTime = 0.0f;

    Game game;
    game.SetContext(context);

    renderer.SetCamera(camera);

    auto previousTime = std::chrono::steady_clock::now();
    game.Start();

    while (!m_Window.ShouldClose()) {
        m_InputManager.BeginFrame();
        m_Window.PollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        const auto currentTime = std::chrono::steady_clock::now();
        const std::chrono::duration<float> delta = currentTime - previousTime;
        previousTime = currentTime;

        game.Update(delta.count());
        game.Render(renderer);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        m_Window.SwapBuffers();
    }
}

void Application::Shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
