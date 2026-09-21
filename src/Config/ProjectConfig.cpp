
#include "ProjectConfig.h"
#include "Logger/LoggerService.h"
#include "IO/VFS/VFS.h"
#include <fstream>
#include <nlohmann/json.hpp>

#pragma push_macro("LOG_WHO")
#define LOG_WHO "ProjectConfig"

namespace {
    Config::ProjectConfig ParseProjectConfig(const nlohmann::json &document) {
        Config::ProjectConfig config;

        if (const auto id = document.find("UUID"); id != document.end() && id->is_string()) {
            config.UUID = id->get<std::string>();
        }

        if (const auto window = document.find("window"); window != document.end() && window->is_object()) {
            if (const auto title = window->find("title"); title != window->end() && title->is_string()) {
                config.Title = title->get<std::string>();
            }
        }

        if (const auto startScene = document.find("start_scene"); startScene != document.end() && startScene->is_string()) {
            config.startScenePath = startScene->get<std::string>();
        }

        return config;
    }
} // namespace

namespace Config {
    ProjectConfig ProjectConfig::Deserialize(const std::string &filepath) {
        const auto document = IO::VFS::ReadVirtualJson(filepath);
        if (!document) {
            LOG_WARN(LOG_WHO, "Could not load virtual project configuration: " + filepath + ". Using defaults.");
            return {};
        }

        return ParseProjectConfig(*document);
    }

    std::optional<ProjectConfig> ProjectConfig::DeserializeFile(const std::filesystem::path &filepath) {
        try {
            std::ifstream file(filepath, std::ios::binary);
            if (!file) {
                LOG_WARN(LOG_WHO, "Could not open project file: " + filepath.string());
                return std::nullopt;
            }

            nlohmann::json document;
            file >> document;

            if (file.bad()) {
                LOG_ERROR(LOG_WHO, "Failed while reading project file: " + filepath.string());
                return std::nullopt;
            }

            return ParseProjectConfig(document);
        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, "Failed to parse project file '" + filepath.string() + "': " + e.what());
            return std::nullopt;
        }
    }

    bool ProjectConfig::Serialize(const ProjectConfig &conf, const std::string &filepath) {
        try {
            nlohmann::json j;
            j["UUID"] = conf.UUID;
            j["window"]["title"] = conf.Title;
            j["start_scene"] = conf.startScenePath;

            std::filesystem::path resolvedPath = IO::VFS::Resolve(filepath);
            std::ofstream file(resolvedPath);

            if (!file.is_open()) {
                LOG_ERROR(LOG_WHO, "Failed to open project file for writing: " + resolvedPath.string());
                return false;
            }
            file << j.dump(2);
            if (!file) {
                LOG_ERROR(LOG_WHO, "Failed to write project file: " + resolvedPath.string());
                return false;
            }
            return true;
        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, "Failed to serialize project file: " + std::string(e.what()));
            return false;
        }
    }
} // namespace Config
#pragma pop_macro("LOG_WHO")
