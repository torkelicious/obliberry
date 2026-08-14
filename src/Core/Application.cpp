#include "Application.h"
#include "Core/EngineContext.h"
#include "Logger/LoggerService.h"
#include "Rendering/MeshFactory.h"
#include "Rendering/InternalShaders.h"
#include "Rendering/Renderer.h"
#include "Sound/AudioEngine.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <nfd.hpp>
#include <thread>
#include <utility>
#include "Applications/Editor/EditorLayer.h"


Core::Application::Application(const Config::GraphicsConfig gconf, Config::ProjectConfig pconf, std::unique_ptr<ApplicationLayer> layer)
    : m_Project(std::move(pconf)), m_GraphicsConfig(gconf), m_Window(m_GraphicsConfig.WindowWidth, m_GraphicsConfig.WindowHeight, m_Project.Title.c_str(), &gconf), m_Layer(std::move(layer)) {
    m_Window.SetInputManager(&m_InputManager);
    m_AudioEngine = Sound::AudioEngine::Create();
}

static void UpdateImDrawData(ImDrawData *&dst, const ImDrawData *src) {
    if (!dst) {
        dst = IM_NEW(ImDrawData)();
    }

    for (int i = 0; i < dst->CmdListsCount; ++i) {
        IM_DELETE(dst->CmdLists[i]);
    }
    dst->CmdLists.clear();

    if (!src || src->CmdListsCount == 0) {
        dst->Valid = false;
        dst->CmdListsCount = 0;
        dst->TotalIdxCount = 0;
        dst->TotalVtxCount = 0;
        return;
    }

    dst->Valid = src->Valid;
    dst->CmdListsCount = src->CmdListsCount;
    dst->TotalIdxCount = src->TotalIdxCount;
    dst->TotalVtxCount = src->TotalVtxCount;
    dst->DisplayPos = src->DisplayPos;
    dst->DisplaySize = src->DisplaySize;
    dst->FramebufferScale = src->FramebufferScale;
    dst->OwnerViewport = src->OwnerViewport;
    dst->CmdLists.resize(src->CmdListsCount);
    for (int i = 0; i < src->CmdListsCount; ++i) {
        dst->CmdLists[i] = src->CmdLists[i]->CloneOutput();
    }
    dst->Textures = src->Textures;
}

static void FreeImDrawData(ImDrawData *data) {
    if (!data)
        return;
    for (int i = 0; i < data->CmdListsCount; ++i) {
        IM_DELETE(data->CmdLists[i]);
    }
    data->CmdLists.clear();
    IM_DELETE(data);
}

void Core::Application::Run() {
    // GLFW
    glfwMakeContextCurrent(m_Window.GetNativeWindow());
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (m_GraphicsConfig.MSAAEnabled)
        glEnable(GL_MULTISAMPLE);

    glfwSwapInterval(static_cast<int>(m_GraphicsConfig.VSync));

    {
        // ImGui Setup
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplGlfw_InitForOpenGL(m_Window.GetNativeWindow(), true);
        ImGui_ImplOpenGL3_Init();
        ImGui::StyleColorsDark();
        ImGuiStyle &style = ImGui::GetStyle();
        ImGuiIO &io = ImGui::GetIO();
        // dpi
        style.FontSizeBase = 20.0f;
        io.ConfigDpiScaleFonts = true;
        io.ConfigDpiScaleViewports = true;
        // features
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;        // Docking
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;    // Keyboard UI navigation
        io.ConfigFlags |= ImGuiConfigFlags_NavNoCaptureKeyboard; // Don't capture keyboard for navigation
    }

    // Renderer
    Rendering::Renderer renderer;
    m_UIRenderer.InitGL();
    m_UIRenderer.SetGameResolution(m_GraphicsConfig.WindowWidth, m_GraphicsConfig.WindowHeight);
    Rendering::Camera camera;

    // EngineContext Setup
    EngineContext context;
    context.projectConfig = &m_Project;
    context.graphicsConfig = &m_GraphicsConfig;
    context.window = &m_Window;
    context.input = &m_InputManager;
    context.resources = &ResourceManager::GetInstance();
    context.renderer = &renderer;
    context.uiRenderer = &m_UIRenderer;
    context.camera = &camera;
    context.deltaTime = 0.0f;
    context.scriptPool = &m_ScriptPool;
    context.threadPool = &m_ThreadPool;
    context.audioEngine = m_AudioEngine.get();
    context.logger = Logging::LoggerService::Get();

    // MSAA supported samples, must be called before gl context handover!!!
    Config::GraphicsCapabilities::CacheSampleCounts();

    // Meshes
    Rendering::MeshFactory::RegisterAllMeshFactories();

    // engine builtin shaders (light pass, etc)
    Rendering::BuiltinShaders::RegisterBuiltinShaders(ResourceManager::GetInstance());
    renderer.SetFallbackShader(ResourceManager::GetInstance().Get<Rendering::Shader>("[Engine] Base").get());

    // engine builtin meshes + default material
    Rendering::BuiltinShaders::RegisterBuiltinAssets(ResourceManager::GetInstance());

    // Camera
    const float initialAspect = static_cast<float>(m_Window.GetWidth()) / static_cast<float>(m_Window.GetHeight());
    renderer.SetCamera(camera, initialAspect);

    // DT
    auto previousTime = std::chrono::steady_clock::now();

    // Layer
    m_Layer->Init(context);

    // ImGui setup.. again
    ImGui_ImplOpenGL3_CreateDeviceObjects();

    // context handover
    glfwMakeContextCurrent(nullptr);

    // Main loop
    m_Running = true;
    m_RenderThread = std::thread(&Application::RenderThreadWorker, this, &renderer, &m_UIRenderer);

    while (!m_Window.ShouldClose()) {
        m_InputManager.BeginFrame();
        Platform::Window::Window::PollEvents();

        // imgui lock
        std::unique_lock imguiTextureLock(m_ImGuiTextureMutex);

        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // delta time
        const auto currentTime = std::chrono::steady_clock::now();
        const std::chrono::duration<float> delta = currentTime - previousTime;
        previousTime = currentTime;

        m_Layer->Update(delta.count());
        if (context.audioEngine) {
            context.audioEngine->Update();
        }

        m_UIRenderer.BeginFrame(m_Window.GetWidth(), m_Window.GetHeight());
        m_Layer->Render();
        ImGui::Render();

        // prolly safe to unlock
        imguiTextureLock.unlock();

        renderer.SwapBuffers();
        m_UIRenderer.SwapBuffers();

        const int writeIdx = m_MainFrameIndex;
        UpdateImDrawData(m_FrameImGuiData[writeIdx], ImGui::GetDrawData());

        // hand off frame to render thread
        {
            std::lock_guard lock(m_Frames[writeIdx].mutex);
            m_Frames[writeIdx].state = FrameState::Ready;
        }
        {
            std::lock_guard lock(m_RenderMutex);
            m_ReadyFrames.push_back(writeIdx);
        }
        m_RenderCV.notify_one();

        // determine next write buffer and ensure availability
        int nextIdx = (writeIdx + 1) % 2;
        {
            std::unique_lock lock(m_Frames[nextIdx].mutex);
            if (m_Frames[nextIdx].state != FrameState::Free) {
                m_Frames[nextIdx].cv.wait(lock, [&] { return m_Frames[nextIdx].state == FrameState::Free || !m_Running.load(); });
            }
        }
        if (!m_Running)
            break;
        m_MainFrameIndex = nextIdx;
    }

    // shutdown
    m_Running = false;
    for (auto &m_Frame : m_Frames) {
        std::lock_guard lock(m_Frame.mutex);
        if (m_Frame.state != FrameState::Free) {
            m_Frame.state = FrameState::Free;
        }
    }
    {
        std::lock_guard lock(m_RenderMutex);
        m_ReadyFrames.clear();
    }
    m_RenderCV.notify_all();

    if (m_RenderThread.joinable()) {
        m_RenderThread.join();
    }

    // free ImGui draw data after render thread has exited
    for (auto &i : m_FrameImGuiData) {
        if (i) {
            FreeImDrawData(i);
            i = nullptr;
        }
    }
}

void Core::Application::Shutdown() const {
    m_Layer->Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Core::Application::RenderThreadWorker(Rendering::Renderer *renderer, UI::UIRenderer *uiRenderer) {
    glfwMakeContextCurrent(m_Window.GetNativeWindow());

    // Enable VSync specifically for this thread's context
    // some drivers handle it weird so better to be safe..
    glfwSwapInterval(static_cast<int>(m_GraphicsConfig.VSync));

    using Clock = std::chrono::steady_clock;
    auto frameStart = Clock::now();
    const auto targetFrameTime = std::chrono::microseconds(m_GraphicsConfig.TargetFPS > 0 ? static_cast<long long>(1000000.0f / m_GraphicsConfig.TargetFPS) : 0);
    const bool frameLimit = m_GraphicsConfig.TargetFPS > 0 && m_GraphicsConfig.VSync == Config::VSyncType::NONE;

    while (m_Running) {
        int frameIdx = 0;
        {
            std::unique_lock lock(m_RenderMutex);
            m_RenderCV.wait(lock, [&] { return !m_ReadyFrames.empty() || !m_Running.load(); });
            if (!m_Running && m_ReadyFrames.empty())
                break;
            frameIdx = m_ReadyFrames.front();
            m_ReadyFrames.pop_front();
        }

        {
            std::lock_guard lock(m_Frames[frameIdx].mutex);
            if (m_Frames[frameIdx].state != FrameState::Ready) {
                continue;
            }
            m_Frames[frameIdx].state = FrameState::Rendering;
        }

        // Wayland zerosize safeguard
        if (m_Window.GetWidth() == 0 || m_Window.GetHeight() == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));

            std::lock_guard lock(m_Frames[frameIdx].mutex);
            m_Frames[frameIdx].state = FrameState::Free;
            m_Frames[frameIdx].cv.notify_one();
            continue;
        }
        Rendering::Renderer::ProcessInitQ();

        if (const auto fbo = renderer->GetEditorFramebuffer()) {
            // Render to FrameBuffer if editor mode
            fbo->Bind();
            Rendering::Renderer::ApplyClearColor();
            glClear(GL_COLOR_BUFFER_BIT);

            renderer->Flush(static_cast<size_t>(frameIdx));

            fbo->BindDrawBuffers();

            uiRenderer->Flush(fbo->GetWidth(), fbo->GetHeight());

            fbo->ClearEntityIDAttachment();
            fbo->Unbind();
            glViewport(0, 0, m_Window.GetWidth(), m_Window.GetHeight());
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        } else {
            glViewport(0, 0, m_Window.GetWidth(), m_Window.GetHeight());
            Rendering::Renderer::ApplyClearColor();
            glClear(GL_COLOR_BUFFER_BIT);

            renderer->Flush(static_cast<size_t>(frameIdx));
        }

        if (m_FrameImGuiData[frameIdx] && m_FrameImGuiData[frameIdx]->CmdListsCount > 0) {
            std::lock_guard imguiTextureLock(m_ImGuiTextureMutex);
            ImGui_ImplOpenGL3_RenderDrawData(m_FrameImGuiData[frameIdx]);
        }

        if (!renderer->GetEditorFramebuffer()) {
            uiRenderer->Flush();
        }

        m_Window.SwapBuffers();

        // frame limiter only when VSync is off to avoid burning CPU for invisible frames
        if (frameLimit) {
            if (const auto elapsed = Clock::now() - frameStart; elapsed < targetFrameTime) {
                std::this_thread::sleep_for(targetFrameTime - elapsed);
            }
        }
        frameStart = Clock::now(); // start timing the next frame

        {
            std::lock_guard lock(m_Frames[frameIdx].mutex);
            m_Frames[frameIdx].state = FrameState::Free;
        }
        m_Frames[frameIdx].cv.notify_one();
    }

    // clean up any leftover
    for (auto &m_Frame : m_Frames) {
        std::lock_guard lock(m_Frame.mutex);
        if (m_Frame.state == FrameState::Ready || m_Frame.state == FrameState::Rendering) {
            m_Frame.state = FrameState::Free;
        }
    }
    ImGui_ImplOpenGL3_Shutdown();
    glfwMakeContextCurrent(nullptr);
}
