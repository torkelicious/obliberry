#include "GraphicsConfig.h"
#include "Core/Utils/PathUtils.h"
#include "Logger/LoggerService.h"
#include "IO/VFS/VFS.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <cmath>
#include <stdexcept>

#pragma push_macro("LOG_WHO")
#define LOG_WHO "GraphicsConfig"

namespace Config {
    std::vector<uint8_t> GraphicsCapabilities::s_SupportedSampleCounts;

    // VSync JSON string mapping
    const char *GraphicsConfig::VSyncToString(const VSyncType vsync) {
        switch (vsync) {
            case VSyncType::NONE:
                return "none";
            case VSyncType::STANDARD:
                return "standard";
            case VSyncType::ADAPTIVE:
                return "adaptive";
        }
        return "standard";
    }

    static VSyncType StringToVSync(const std::string &str) {
        if (str == "none")
            return VSyncType::NONE;
        if (str == "adaptive")
            return VSyncType::ADAPTIVE;
        return VSyncType::STANDARD;
    }

    uint8_t GraphicsConfig::SnapToValidSampleCount(const uint8_t requested, const std::vector<uint8_t> &validSamples) {
        if (validSamples.empty())
            return requested;

        uint8_t closest = validSamples[0];
        int smallestDiff = std::abs(static_cast<int>(requested) - static_cast<int>(closest));
        for (const uint8_t candidate : validSamples) {
            if (const int diff = std::abs(static_cast<int>(requested) - static_cast<int>(candidate)); diff < smallestDiff) {
                smallestDiff = diff;
                closest = candidate;
            }
        }
        return closest;
    }

    static GraphicsConfig ParseGraphicsConfig(const nlohmann::json &j) {
        if (!j.is_object()) {
            throw std::runtime_error("Graphics config must be a JSON object.");
        }

        GraphicsConfig parsed;
        if (j.contains("window")) {
            const auto &w = j.at("window");
            if (!w.is_object()) {
                throw std::runtime_error("Graphics window settings must be an object.");
            }
            if (w.contains("width")) {
                parsed.WindowWidth = w.at("width").get<int>();
            }
            if (w.contains("height")) {
                parsed.WindowHeight = w.at("height").get<int>();
            }
            if (w.contains("fullscreen")) {
                parsed.Fullscreen = w.at("fullscreen").get<bool>();
            }
        }
        if (j.contains("antialiasing")) {
            const auto &aa = j.at("antialiasing");
            if (!aa.is_object()) {
                throw std::runtime_error("Graphics antialiasing settings must be an object.");
            }
            if (aa.contains("MSAA")) {
                parsed.MSAAEnabled = aa.at("MSAA").get<bool>();
            }
            if (aa.contains("samples")) {
                parsed.AASamples = aa.at("samples").get<uint8_t>();
            }
        }
        if (j.contains("targetfps")) {
            parsed.TargetFPS = j.at("targetfps").get<int>();
        }
        if (j.contains("vsync")) {
            const auto &vsync = j.at("vsync");
            if (vsync.is_string()) {
                parsed.VSync = StringToVSync(vsync.get<std::string>());
            } else if (vsync.is_boolean()) {
                parsed.VSync = vsync.get<bool>() ? VSyncType::STANDARD : VSyncType::NONE;
            } else {
                throw std::runtime_error("Graphics vsync must be a string or boolean.");
            }
        }
        if (j.contains("overlay")) {
            parsed.ShowPerformanceOverlay = j.at("overlay").get<bool>();
        }
        return parsed;
    }

    static void LogGraphicsConfig(const GraphicsConfig &config) {
        LOG_INFO(LOG_WHO, "Loaded graphics config:");
        LOG_INFO(LOG_WHO, "  Window:        " + std::to_string(config.WindowWidth) + "x" + std::to_string(config.WindowHeight) + (config.Fullscreen ? " (fullscreen)" : ""));
        LOG_INFO(LOG_WHO, "  VSync:         " + std::string(GraphicsConfig::VSyncToString(config.VSync)));
        LOG_INFO(LOG_WHO, "  Target FPS:    " + std::to_string(config.TargetFPS));
        LOG_INFO(LOG_WHO, "  MSAA:          " + std::string(config.MSAAEnabled ? "on (" + std::to_string(config.AASamples) + "x)" : "off"));
        LOG_INFO(LOG_WHO, "  FPS Overlay:   " + std::string(config.ShowPerformanceOverlay ? "on" : "off"));
    }

    GraphicsConfig GraphicsConfig::Deserialize(const std::filesystem::path &filepath) {
        if (IO::VFS::IsPackaged() && filepath.is_relative()) {
            try {
                const auto overridePath = Core::PathUtils::GetExecutableDirectory() / filepath;
                std::ifstream file(overridePath);
                if (file.is_open()) {
                    const auto j = nlohmann::json::parse(file);
                    auto config = ParseGraphicsConfig(j);
                    LOG_INFO(LOG_WHO, "Loaded graphics config : " + overridePath.string());
                    LogGraphicsConfig(config);
                    return config;
                }
            } catch (const std::exception &e) {
                LOG_WARN(LOG_WHO, "Could not load graphics override, trying through VFS: " + std::string(e.what()));
            }
        }

        const auto data = IO::VFS::ReadVirtualJson(filepath);
        if (!data.has_value()) {
            LOG_WARN(LOG_WHO, "Graphics config not found: " + filepath.string() + ". Using defaults");
            return GraphicsConfig{};
        }
        try {
            auto config = ParseGraphicsConfig(*data);
            LogGraphicsConfig(config);
            return config;
        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, "Failed to parse graphics config: " + std::string(e.what()));
            return GraphicsConfig{};
        }
    }

    void GraphicsConfig::Serialize(const GraphicsConfig &conf, const std::filesystem::path &filepath) {
        try {
            nlohmann::json j;
            j["window"]["width"] = conf.WindowWidth;
            j["window"]["height"] = conf.WindowHeight;
            j["window"]["fullscreen"] = conf.Fullscreen;
            j["antialiasing"]["MSAA"] = conf.MSAAEnabled;
            j["antialiasing"]["samples"] = SnapToValidSampleCount(conf.AASamples, GraphicsCapabilities::s_SupportedSampleCounts);
            j["targetfps"] = conf.TargetFPS;
            j["vsync"] = VSyncToString(conf.VSync);
            j["overlay"] = conf.ShowPerformanceOverlay;

            std::filesystem::path resolvedPath = IO::VFS::Resolve(filepath.string());
            std::ofstream file(resolvedPath);
            if (!file.is_open()) {
                LOG_ERROR(LOG_WHO, "Failed to open graphics config for writing: " + resolvedPath.string());
                return;
            }
            file << j.dump(2);
            if (!file) {
                LOG_ERROR(LOG_WHO, "Failed to write graphics config: " + resolvedPath.string());
                return;
            }
            LOG_INFO(LOG_WHO, "Saved graphics config to " + resolvedPath.string());
        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, "Failed to serialize graphics config: " + std::string(e.what()));
        }
    }

} // namespace Config

#pragma pop_macro("LOG_WHO")
