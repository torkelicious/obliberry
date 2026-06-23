#pragma once
#include <string>

#include "Constants.h"

struct ProjectConfig {
    std::string windowTitle = "Obliberry Project";
    std::string startScenePath = "assets/scenes/default.json";
    int windowWidth = WINDOW_WIDTH;
    int windowHeight = WINDOW_HEIGHT;
    bool fullscreen = false;

    static ProjectConfig Deserialize(const std::string &filepath = "project.json");
};
