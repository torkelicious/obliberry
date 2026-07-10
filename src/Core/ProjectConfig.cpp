#include "ProjectConfig.h"
#include "Core/LoggerService.h"
#include "IO/VFS.h"
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>

namespace Core {

    constexpr auto LOG_WHO = "ProjectConfig";
    ProjectConfig ProjectConfig::Deserialize(const std::string &filepath) {
        ProjectConfig config;

        auto fileData = IO::VFS::ReadVirtual(filepath);
        if (!fileData.has_value()) {
            LOG_WARN(LOG_WHO, "Project file not found in VFS: " + filepath + ". Using defaults");
            return config;
        }

        try {
            nlohmann::json j;

            if (IO::VFS::IsPackaged()) {
                std::vector<uint8_t> bytes(fileData.value().begin(), fileData.value().end());
                j = nlohmann::json::from_msgpack(bytes);
            } else {
                j = nlohmann::json::parse(fileData.value());
            }

            if (j.contains("window")) {
                auto &w = j["window"];
                if (w.contains("width"))
                    config.windowWidth = w["width"];
                if (w.contains("height"))
                    config.windowHeight = w["height"];
                if (w.contains("title"))
                    config.Title = w["title"];
                if (w.contains("fullscreen"))
                    config.fullscreen = w["fullscreen"];
            }
            if (j.contains("start_scene")) {
                config.startScenePath = j["start_scene"];
            }
        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, "Failed to parse project file: " + std::string(e.what()));
        }

        return config;
    }

    bool ProjectConfig::Serialize(const ProjectConfig &conf, const std::string &filepath) {
        try {
            nlohmann::json j;
            j["window"]["width"] = conf.windowWidth;
            j["window"]["height"] = conf.windowHeight;
            j["window"]["title"] = conf.Title;
            j["window"]["fullscreen"] = conf.fullscreen;
            j["start_scene"] = conf.startScenePath;

            std::filesystem::path resolvedPath = IO::VFS::Resolve(filepath);
            std::ofstream file(resolvedPath);

            if (!file.is_open()) {
                LOG_ERROR(LOG_WHO, "Failed to open project file for writing: " + resolvedPath.string());
                return false;
            }
            file << j.dump(2);
            return true;
        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, "Failed to serialize project file: " + std::string(e.what()));
            return false;
        }
    }
} // namespace Core
