#pragma once

#include <condition_variable>
#include <mutex>
#include <thread>
#include <atomic>
#include <memory>

#include "ProjectConfig.h"
#include "ResourceManager.h"
#include "Window.h"
#include "Game/Game.h"
#include "Renderer/Renderer.h"
#include "Scripting/ObSLCore/Interpreter/Interpreter.h"
#include "Sound/AudioEngine.h"

struct ImDrawData;

class Application {
public:
    explicit Application(ProjectConfig config);

    ~Application() {
        Shutdown();
    }

    void Run();

    static void Shutdown();

private:
    enum class FrameState : uint8_t {
        Free,
        Ready,
        Rendering
    };

    struct FrameSync {
        std::mutex mutex;
        std::condition_variable cv;
        FrameState state = FrameState::Free;
    };

    void RenderThreadWorker(Renderer *renderer, Camera *camera, Game *game);

    ProjectConfig m_Project;
    Window m_Window;
    InputManager m_InputManager;
    ResourceManager m_ResourceManager;
    ObSL::Interpreter m_ScriptEngine;
    std::unique_ptr<AudioEngine> m_AudioEngine;

    std::thread m_RenderThread;
    std::mutex m_RenderMutex;
    std::condition_variable m_RenderCV;

    std::atomic<bool> m_Running{true};

    FrameSync m_Frames[2];
    int m_MainFrameIndex = 0;

    ImDrawData *m_FrameImGuiData[2] = {nullptr, nullptr};
};
