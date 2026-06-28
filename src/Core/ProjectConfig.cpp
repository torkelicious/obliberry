#include "ProjectConfig.h"
#include "IO/VFS.h"
#include <fstream>
#include <iostream>
#include <json.hpp>

ProjectConfig ProjectConfig::Deserialize(const std::string &filepath) {
    ProjectConfig config;
    std::filesystem::path resolvedPath = IO::VFS::Resolve(filepath);
    std::ifstream file(resolvedPath);

    if (!file.is_open()) {
        std::cerr << "[ProjectConfig] Project file not found: " << filepath
                << " (Resolved: " << resolvedPath.string() << "). Using defaults.\n";
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
    } catch (const std::exception &e) {
        std::cerr << "[ProjectConfig] Failed to parse project file: " << e.what() << "\n";
    }
    return config;
}

bool ProjectConfig::Serialize(const ProjectConfig &conf, const std::string &filepath) {
    try {
        nlohmann::json j;
        j["window"]["width"] = conf.windowWidth;
        j["window"]["height"] = conf.windowHeight;
        j["window"]["title"] = conf.windowTitle;
        j["window"]["fullscreen"] = conf.fullscreen;
        j["start_scene"] = conf.startScenePath;
        std::filesystem::path resolvedPath = IO::VFS::Resolve(filepath);
        std::ofstream file(resolvedPath);

        if (!file.is_open()) {
            std::cerr << "[ProjectConfig] Failed to open project file for writing: " << resolvedPath.string() << "\n";
            return false;
        }
        file << j.dump(2);
        return true;
    } catch (const std::exception &e) {
        std::cerr << "[ProjectConfig] Failed to serialize project file: " << e.what() << "\n";
        return false;
    }
}
