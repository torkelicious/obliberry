#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace Config {

    enum class SaveLocation : std::uint8_t { DataHome = 0, Portable = 1 };

    struct ProjectConfig {
        std::string UUID;
        std::string Title = "Obliberry Project";
        std::string startScenePath;

        SaveLocation saveLocation = SaveLocation::DataHome;
        bool useSaves = false;

        static ProjectConfig Deserialize(const std::string &filepath = "project.json");

        static std::optional<ProjectConfig> DeserializeFile(const std::filesystem::path &filepath);

        static bool Serialize(const ProjectConfig &conf, const std::string &filepath = "project.json");
    };

} // namespace Config
