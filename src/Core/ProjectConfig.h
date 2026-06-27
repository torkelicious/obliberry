#pragma once
#include <string>
#include "Constants.h"

struct ProjectConfig {
    std::string windowTitle = "Obliberry Project";
    std::string startScenePath = "assets/scenes/default.json";
    int windowWidth = WINDOW_WIDTH;
    int windowHeight = WINDOW_HEIGHT;
    bool fullscreen = false;

    // default to the central relative project tag identifier
    static ProjectConfig Deserialize(const std::string &filepath = "project.json");

    static bool Serialize(const ProjectConfig &conf, const std::string &filepath = "project.json");
};
