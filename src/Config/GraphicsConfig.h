#pragma once
#include "Core/Constants.h"
#include "glad/glad.h"
#include <cstdint>
#include <bits/stl_vector.h>

namespace Config {

    struct GraphicsCapabilities {
        static std::vector<uint8_t> s_SupportedSampleCounts;

        // must be called from valid GL context!!!!!!!!!!!!!
        static std::vector<uint8_t> QuerySupportedSampleCounts() {
            GLint maxSamples = 0;
            glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
            static const uint8_t candidates[] = {1, 2, 4, 8, 16};
            std::vector<uint8_t> supported;
            for (uint8_t c : candidates) {
                if (c <= maxSamples)
                    supported.push_back(c);
            }
            if (supported.empty())
                supported.push_back(1);

            return supported;
        }

        static void CacheSampleCounts() { s_SupportedSampleCounts = QuerySupportedSampleCounts(); }
    };

    enum class VSyncType : int8_t {
        // matches the GLFW value !!!
        ADAPTIVE = -1,
        NONE = 0,
        STANDARD = 1,
    };

    struct GraphicsConfig {
        int TargetFPS = 60; // For frame limiter
        // Window aspect
        int WindowWidth = Core::WINDOW_WIDTH;
        int WindowHeight = Core::WINDOW_HEIGHT;
        VSyncType VSync = VSyncType::STANDARD;

        bool ShowPerformanceOverlay = false; // only applies in runtime!!!
        bool MSAAEnabled = false;
        bool Fullscreen = false;
        uint8_t AASamples = 4; // if MSAA on, how many samples

        static const char *VSyncToString(VSyncType vsync);
        static uint8_t SnapToValidSampleCount(uint8_t requested, const std::vector<uint8_t> &validSamples = GraphicsCapabilities::s_SupportedSampleCounts);
        static GraphicsConfig Deserialize(const std::string &filepath = "graphics.json");
        static void Serialize(const GraphicsConfig &conf, const std::string &filepath = "graphics.json");
    };

} // namespace Config
