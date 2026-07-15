#pragma once
#include "Core/Constants.h"
#include <string>

namespace Config {
    struct ProjectConfig {
        std::string Title = "Obliberry Project";
        std::string startScenePath;


        // default to the central relative project tag identifier
        static ProjectConfig Deserialize(const std::string &filepath = "project.json");

        static bool Serialize(const ProjectConfig &conf, const std::string &filepath = "project.json");
    };
} // namespace Config
