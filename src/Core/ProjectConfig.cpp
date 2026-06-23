#include "ProjectConfig.h"

#include <fstream>
#include <iostream>
#include <json.hpp>

ProjectConfig ProjectConfig::Deserialize(const std::string &filepath) {
    ProjectConfig config;
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Project file not found: " << filepath << ". Attempting to use defaults.\n";
        return config;
    }
    try {
        nlohmann::json j;
        file >> j;
        if (j.contains("window")) {
            auto &w = j["window"];
            if (w.contains("width")) config.windowWidth = w["width"];
            if (w.contains("height")) config.windowHeight = w["height"];
            if (w.contains("title")) config.windowTitle = w["title"];
            if (w.contains("fullscreen")) config.fullscreen = w["fullscreen"];
        }
        if (j.contains("start_scene")) {
            config.startScenePath = j["start_scene"];
        }
    } catch
    (const std::exception &e) {
        std::cerr << "Failed to parse project file: " << e.what() << "\n";
    }
    return config;
}
