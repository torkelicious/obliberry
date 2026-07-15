#pragma once
#include "Core/Constants.h"


#include <cstdint>
namespace Config {

    enum class VSyncType : int8_t {
        // matches the GLFW value !!!
        ADAPTIVE = -1,
        NONE = 0,
        STANDARD = 1,
    };

    struct GraphicsConfig {
        float TargetFPS = 60; // For frame limiter
        // Window aspect
        int WindowWidth = Core::WINDOW_WIDTH;
        int WindowHeight = Core::WINDOW_HEIGHT;
        VSyncType VSync = VSyncType::STANDARD;

        bool MSAAEnabled = false;
        bool Fullscreen = false;
        uint8_t AASamples = 4; // if MSAA on, how many samples

        static GraphicsConfig Deserialize(const std::string &filepath = "graphics.json");
        static void Serialize(const GraphicsConfig &conf, const std::string &filepath = "graphics.json");
    };

} // namespace Config
