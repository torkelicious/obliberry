#pragma once

#include <condition_variable>
#include <mutex>
#include <thread>
#include <atomic>
#include <deque>
#include <memory>
#include "Config/ProjectConfig.h"
#include "ResourceManager.h"
#include "Platform/Window/Window.h"
#include "ApplicationLayer.h"
#include "Config/GraphicsConfig.h"
#include <ObSL/ScriptRuntime.h>
#include "Platform/Threading/ThreadPool.h"
#include "Sound/AudioEngine.h"

struct ImDrawData;

namespace Core {
    class Application {
    public:
        explicit Application(Config::GraphicsConfig gconf, Config::ProjectConfig pconf, std::unique_ptr<ApplicationLayer> layer);

        ~Application() { Shutdown(); }

        void Run();

        void Shutdown() const;

    private:
        enum class FrameState : uint8_t { Free, Ready, Rendering };

        struct FrameSync {
            std::mutex mutex;
            std::condition_variable cv;
            FrameState state = FrameState::Free;
        };

        void RenderThreadWorker(Rendering::Renderer *renderer);

        Config::ProjectConfig m_Project;
        Config::GraphicsConfig m_GraphicsConfig;
        Platform::Window::Window m_Window;
        Platform::Input::InputManager m_InputManager;
        ResourceManager m_ResourceManager;
        ObSL::ScriptRuntime m_ScriptPool;
        Platform::Threading::ThreadPool m_ThreadPool;
        std::unique_ptr<Sound::AudioEngine> m_AudioEngine;

        std::unique_ptr<ApplicationLayer> m_Layer;

        std::thread m_RenderThread;
        std::mutex m_RenderMutex;
        std::condition_variable m_RenderCV;
        std::deque<int> m_ReadyFrames;

        std::atomic<bool> m_Running{true};

        FrameSync m_Frames[2];
        int m_MainFrameIndex = 0;

        ImDrawData *m_FrameImGuiData[2] = {nullptr, nullptr};
    };
} // namespace Core
