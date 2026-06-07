#include "Application.h"
#include <chrono>
#include <thread>
#include "Constants.h"
#include "Game/Game.h"
#include "Renderer/Renderer.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


Application::Application()
    : m_Window(WINDOW_WIDTH, WINDOW_HEIGHT, "Window") {
    m_Window.SetInputManager(&m_InputManager);
}

void Application::Run() {
    // move this to some sort of Init outside of Application
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glfwSwapInterval(1);

    // gui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(m_Window.GetNativeWindow(), true);
    ImGui_ImplOpenGL3_Init();
    ImGui::StyleColorsDark();

    Renderer renderer;
    Camera camera;
    Game game;
    game.SetWindow(&m_Window);
    game.SetInputManager(&m_InputManager);
    game.SetCamera(&camera);
    renderer.SetCamera(camera, WINDOW_WIDTH, WINDOW_HEIGHT);


    auto previousTime = std::chrono::steady_clock::now();
    game.Start();

    // tile gen number
    int i = 20;
    int p = 50;
    while (!m_Window.ShouldClose()) {
        // input
        m_InputManager.BeginFrame();
        // glfw
        m_Window.PollEvents();

        // gui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("obliberry"); // Create an imgui window
        // append fps counter to window
        ImGui::Text("average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        // fun sliders...
        ImGui::SliderInt("Hex Tiles", &i, 1, 100);
        ImGui::SliderInt("Grid Sand percentage", &p, 0, 100);
        if (ImGui::Button("Regenerate Grid")) {
            game.MovePlayerToCenter(); // avoid break pathfinding
            game.GenerateTiles(i, p);
        }
        ImGui::End(); // declare end of this window

        // game
        const auto currentTime = std::chrono::steady_clock::now();
        const std::chrono::duration<float> delta = currentTime - previousTime;
        previousTime = currentTime;

        game.Update(delta.count());
        game.Render(renderer);

        // gui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // glfw
        m_Window.SwapBuffers();
    }
}

void Application::Shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
