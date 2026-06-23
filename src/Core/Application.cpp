#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Application.h"
#include <chrono>
#include <thread>
#include <utility>
#include "Game/Game.h"
#include "Renderer/Renderer.h"
#include "Core/EngineContext.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Sound/AudioEngine.h"

Application::Application(ProjectConfig config)
    : m_Project(std::move(config)),
      m_Window(m_Project.windowWidth, m_Project.windowHeight, m_Project.windowTitle.c_str(), m_Project.fullscreen) {
    m_Window.SetInputManager(&m_InputManager);
    m_AudioEngine = AudioEngine::Create();
}

void Application::Run() {
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glfwSwapInterval(0);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(m_Window.GetNativeWindow(), true);
    ImGui_ImplOpenGL3_Init();
    ImGui::StyleColorsDark();

    float xscale, yscale;
    glfwGetWindowContentScale(m_Window.GetNativeWindow(), &xscale, &yscale);
    ImGui::GetStyle().ScaleAllSizes(xscale);
    ImGuiIO &io = ImGui::GetIO();
    io.FontGlobalScale = xscale;

    Renderer renderer;
    Camera camera;

    EngineContext context;
    context.projectConfig = &m_Project;
    context.window = &m_Window;
    context.input = &m_InputManager;
    context.resources = &m_ResourceManager;
    context.renderer = &renderer;
    context.camera = &camera;
    context.deltaTime = 0.0f;
    context.scriptEngine = &m_ScriptEngine;
    Game game;
    game.SetContext(context);

    float initialAspect = static_cast<float>(m_Window.GetWidth()) / static_cast<float>(m_Window.GetHeight());
    renderer.SetCamera(camera, initialAspect);

    auto previousTime = std::chrono::steady_clock::now();
    game.Start();

    while (!m_Window.ShouldClose()) {
        m_InputManager.BeginFrame();
        Window::PollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        const auto currentTime = std::chrono::steady_clock::now();
        const std::chrono::duration<float> delta = currentTime - previousTime;
        previousTime = currentTime;

        game.Update(delta.count());
        if (context.audioEngine) { context.audioEngine->Update(); }
        game.Render();
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
