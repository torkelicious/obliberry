#include "Application.h"
#include "Core/EngineContext.h"
#include "Logger/LoggerService.h"
#include "Rendering/Types/Mesh/MeshFactory.h"
#include "Rendering/Types/Shader/InternalShaders.h"
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
#include "ECS/Systems/ScriptSystem.h"
#include "Rendering/PostProcessing/InternalPostProcFx.h"


Core::Application::Application(const Config::GraphicsConfig &gconf, Config::ProjectConfig pconf, std::unique_ptr<ApplicationLayer> layer)
    : m_Project(std::move(pconf)), m_GraphicsConfig(gconf), m_Window(m_GraphicsConfig.WindowWidth, m_GraphicsConfig.WindowHeight, m_Project.Title.c_str(), &gconf), m_Layer(std::move(layer)) {
    m_Window.SetInputManager(&m_InputManager);
    m_AudioEngine = Sound::AudioEngine::Create();
    m_FrameImGuiData[0] = std::make_unique<ImDrawDataSnapshot>();
    m_FrameImGuiData[1] = std::make_unique<ImDrawDataSnapshot>();
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
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Docking
        io.NavActive = true;
        io.ConfigNavCaptureKeyboard = false;
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

    ECS::Systems::ScriptSystem::SetupScriptRuntime(m_ScriptPool);

    m_Layer->SetupFontSync(&m_FontsDirty);

    // MSAA supported samples, must be called before gl context handover!!!
    Config::GraphicsCapabilities::CacheSampleCounts();

    // Meshes
    Rendering::MeshFactory::RegisterAllMeshFactories();

    // engine builtin shaders (light pass, etc)
    Rendering::BuiltinShaders::RegisterBuiltinShaders(ResourceManager::GetInstance());

    Rendering::PostProcessing::Builtins::RegisterBuiltinPostProcShaders(renderer);

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
    renderer.SetEditorMode(m_Layer->UsesEditorViewport());

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

        // The only shared ImGui state between threads is the font atlas / backend texture
        // handles
        // the render thread reads them in RenderDrawData, this thread rebuilds them
        // in SyncFonts.
        // everything else operates per-thread or
        // "snapshotted" data and runs outside the lock.

        {
            std::unique_lock imguiTextureLock(m_ImGuiTextureMutex);

            const bool fontsWereDirty = m_FontsDirty.load();
            if (fontsWereDirty) {
                imguiTextureLock.unlock();
                for (auto &[mutex, cv, state] : m_Frames) {
                    std::unique_lock frameLock(mutex);
                    cv.wait(frameLock, [&] { return state == FrameState::Free || !m_Running.load(); });
                }
                imguiTextureLock.lock();
            }

            // rebuilds and consume
            m_Layer->SyncFonts(context, m_ImGuiTextureMutex);

            if (fontsWereDirty)
                m_FontAtlasRevision.fetch_add(1, std::memory_order_release);
        }

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

        renderer.SwapBuffers();
        m_UIRenderer.SwapBuffers();

        const int writeIdx = m_MainFrameIndex;
        m_FrameImGuiData[writeIdx]->SnapUsingSwap(ImGui::GetDrawData(), ImGui::GetTime());

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
    glfwMakeContextCurrent(m_Window.GetNativeWindow());
}

void Core::Application::Shutdown() {
    m_Layer->Shutdown();
    ImGui_ImplGlfw_Shutdown();
    m_FrameImGuiData[0].reset();
    m_FrameImGuiData[1].reset();
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

        if (renderer->IsEditorMode()) {
            // Editor
            if (const auto sceneFbo = renderer->GetSceneFrameBuffer()) {
                sceneFbo->Bind();
                sceneFbo->BindDrawBuffers();
                Rendering::Renderer::ApplyClearColor();
                glClear(GL_COLOR_BUFFER_BIT);

                renderer->Flush(static_cast<size_t>(frameIdx));
                renderer->RunPostProc();

                uiRenderer->Flush(sceneFbo->GetWidth(), sceneFbo->GetHeight());
                sceneFbo->Unbind();

                glViewport(0, 0, m_Window.GetWidth(), m_Window.GetHeight());
                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
            } else {
                glViewport(0, 0, m_Window.GetWidth(), m_Window.GetHeight());
                Rendering::Renderer::ApplyClearColor();
                glClear(GL_COLOR_BUFFER_BIT);

                renderer->Flush(static_cast<size_t>(frameIdx));
            }
        } else {
            // Runtime
            renderer->EnsureSceneFramebufferSize(m_Window.GetWidth(), m_Window.GetHeight());
            if (const auto sceneFbo = renderer->GetSceneFrameBuffer()) {
                sceneFbo->Bind();
                sceneFbo->BindDrawBuffers();
                Rendering::Renderer::ApplyClearColor();
                glClear(GL_COLOR_BUFFER_BIT);

                renderer->Flush(static_cast<size_t>(frameIdx));
                renderer->RunPostProc();
                sceneFbo->Unbind();

                renderer->PresentToScreen(m_Window.GetWidth(), m_Window.GetHeight());
            } else {
                // first frame before the scene framebuffer is created
                glViewport(0, 0, m_Window.GetWidth(), m_Window.GetHeight());
                Rendering::Renderer::ApplyClearColor();
                glClear(GL_COLOR_BUFFER_BIT);

                renderer->Flush(static_cast<size_t>(frameIdx));
            }
        }

        if (m_FrameImGuiData[frameIdx]->DrawData.CmdLists.Size > 0) {
            std::lock_guard imguiTextureLock(m_ImGuiTextureMutex);
            if (const uint64_t revision = m_FontAtlasRevision.load(std::memory_order_acquire); revision != m_RenderedFontAtlasRevision) {
                ImGui_ImplOpenGL3_DestroyDeviceObjects();
                ImGui_ImplOpenGL3_CreateDeviceObjects();
                m_RenderedFontAtlasRevision = revision;
            }
            ImGui_ImplOpenGL3_RenderDrawData(&m_FrameImGuiData[frameIdx]->DrawData);
        }

        if (!renderer->IsEditorMode()) {
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
