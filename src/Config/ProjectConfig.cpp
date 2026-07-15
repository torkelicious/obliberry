#include "ProjectConfig.h"
#include "Logger/LoggerService.h"
#include "IO/VFS/VFS.h"
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>

#pragma push_macro("LOG_WHO")
#define LOG_WHO "ProjectConfig"

namespace Config {
    ProjectConfig ProjectConfig::Deserialize(const std::string &filepath) {
        ProjectConfig config;

        std::string_view dataView;
        std::string ownedData;
        if (const auto view = IO::VFS::ReadVirtualView(filepath)) {
            dataView = *view;
        } else if (auto owned = IO::VFS::ReadVirtual(filepath)) {
            ownedData = std::move(*owned);
            dataView = ownedData;
        } else {
            LOG_WARN(LOG_WHO, "Project file not found in VFS: " + filepath + ". Using defaults");
            return config;
        }

        try {
            nlohmann::json j;

            if (IO::VFS::IsPackaged()) {
                std::vector<uint8_t> bytes(dataView.begin(), dataView.end());
                j = nlohmann::json::from_msgpack(bytes);
            } else {
                j = nlohmann::json::parse(dataView);
            }

            if (j.contains("window")) {
                if (auto &w = j["window"]; w.contains("title"))
                    config.Title = w["title"];
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
            j["window"]["title"] = conf.Title;
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
} // namespace Config
#pragma pop_macro("LOG_WHO")
