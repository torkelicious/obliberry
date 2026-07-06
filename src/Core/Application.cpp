#include "Application.h"
#include "Core/EngineContext.h"
#include "Rendering/MeshFactory.h"
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

#include "Editor/EditorLayer.h"
#include "IO/VFS.h"

Core::Application::Application(ProjectConfig config, std::unique_ptr<ApplicationLayer> layer)
    : m_Project(std::move(config)),
      m_Window(m_Project.windowWidth, m_Project.windowHeight, m_Project.Title.c_str(), m_Project.fullscreen),
      m_Layer(std::move(layer)) {
    m_Window.SetInputManager(&m_InputManager);
    m_AudioEngine = Sound::AudioEngine::Create();
}

// a bit goofy but idk what else 2 do
static ImDrawData *CloneImDrawData(const ImDrawData *src) {
    if (!src)
        return nullptr;
    auto *dst = IM_NEW(ImDrawData)();
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
    return dst;
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
    glfwMakeContextCurrent(m_Window.GetNativeWindow());

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
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Docking
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; for later

    Rendering::Renderer renderer;
    Rendering::Camera camera;

    EngineContext context;
    context.projectConfig = &m_Project;
    context.window = &m_Window;
    context.input = &m_InputManager;
    context.resources = &m_ResourceManager;
    context.renderer = &renderer;
    context.camera = &camera;
    context.deltaTime = 0.0f;
    context.scriptPool = &m_ScriptPool;
    context.threadPool = &m_ThreadPool;
    context.audioEngine = m_AudioEngine.get();

    Rendering::MeshFactory::RegisterAllMeshFactories();

    float initialAspect = static_cast<float>(m_Window.GetWidth()) / static_cast<float>(m_Window.GetHeight());
    renderer.SetCamera(camera, initialAspect);

    auto previousTime = std::chrono::steady_clock::now();

    m_Layer->Init(context);

    ImGui_ImplOpenGL3_CreateDeviceObjects();

    // context handover
    glfwMakeContextCurrent(nullptr);

    m_Running = true;
    m_RenderThread = std::thread(&Application::RenderThreadWorker, this, &renderer);

    while (!m_Window.ShouldClose()) {
        m_InputManager.BeginFrame();
        Window::PollEvents();

        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        const auto currentTime = std::chrono::steady_clock::now();
        const std::chrono::duration<float> delta = currentTime - previousTime;
        previousTime = currentTime;

        m_Layer->Update(delta.count());
        if (context.audioEngine) {
            context.audioEngine->Update();
        }

        m_Layer->Render();
        ImGui::Render();

        renderer.SwapBuffers();

        const int writeIdx = m_MainFrameIndex;
        if (const ImDrawData *imguiDrawData = ImGui::GetDrawData(); imguiDrawData && imguiDrawData->CmdListsCount > 0) {
            m_FrameImGuiData[writeIdx] = CloneImDrawData(imguiDrawData);
        }

        // hand off frame to render thread
        {
            std::lock_guard lock(m_Frames[writeIdx].mutex);
            m_Frames[writeIdx].state = FrameState::Ready;
        }
        m_RenderCV.notify_one();

        // determine next write buffer and ensure availability
        int nextIdx = (writeIdx + 1) % 2;
        {
            std::unique_lock lock(m_Frames[nextIdx].mutex);
            if (m_Frames[nextIdx].state != FrameState::Free) {
                m_Frames[nextIdx].cv.wait(
                        lock, [&] { return m_Frames[nextIdx].state == FrameState::Free || !m_Running.load(); });
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
    for (auto &i : m_FrameImGuiData) {
        if (i) {
            FreeImDrawData(i);
            i = nullptr;
        }
    }
    m_RenderCV.notify_all();

    if (m_RenderThread.joinable()) {
        m_RenderThread.join();
    }
}

void Core::Application::Shutdown() const {
    m_Layer->Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Core::Application::RenderThreadWorker(Rendering::Renderer *renderer) {
    glfwMakeContextCurrent(m_Window.GetNativeWindow());

    while (m_Running) {
        int frameIdx = -1;

        for (int i = 0; i < 2; ++i) {
            std::lock_guard lock(m_Frames[i].mutex);
            if (m_Frames[i].state == FrameState::Ready) {
                m_Frames[i].state = FrameState::Rendering;
                frameIdx = i;
                break;
            }
        }

        if (frameIdx == -1) {
            // sleep until woken
            std::unique_lock lock(m_RenderMutex);
            m_RenderCV.wait(lock, [&] {
                if (!m_Running)
                    return true;
                for (auto &m_Frame : m_Frames) {
                    std::lock_guard lk(m_Frame.mutex);
                    if (m_Frame.state == FrameState::Ready)
                        return true;
                }
                return false;
            });
            if (!m_Running)
                break;
            continue;
        }

        renderer->ProcessInitQ();

        if (const auto fbo = renderer->GetEditorFramebuffer()) {
            // Render to FrameBuffer if editor mode
            fbo->Bind();
            renderer->ApplyClearColor();
            glClear(GL_COLOR_BUFFER_BIT);

            renderer->Flush(static_cast<size_t>(frameIdx));
            fbo->Unbind();
            glViewport(0, 0, m_Window.GetWidth(), m_Window.GetHeight());
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            fbo->ClearEntityIDAttachment();
        } else {
            glViewport(0, 0, m_Window.GetWidth(), m_Window.GetHeight());
            renderer->ApplyClearColor();
            glClear(GL_COLOR_BUFFER_BIT);

            renderer->Flush(static_cast<size_t>(frameIdx));
        }

        if (m_FrameImGuiData[frameIdx]) {
            ImGui_ImplOpenGL3_RenderDrawData(m_FrameImGuiData[frameIdx]);
            FreeImDrawData(m_FrameImGuiData[frameIdx]);
            m_FrameImGuiData[frameIdx] = nullptr;
        }

        m_Window.SwapBuffers();

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
    for (auto &i : m_FrameImGuiData) {
        if (i) {
            FreeImDrawData(i);
            i = nullptr;
        }
    }
    ImGui_ImplOpenGL3_Shutdown();
    glfwMakeContextCurrent(nullptr);
}
