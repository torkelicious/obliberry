#include "GraphicsConfig.h"
#include "Logger/LoggerService.h"
#include "IO/VFS/VFS.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <cmath>

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

    GraphicsConfig GraphicsConfig::Deserialize(const std::filesystem::path &filepath) {
        GraphicsConfig config;
        std::string_view dataView;
        std::string ownedData;

        // Convert path to string for VFS calls that expect string keys/paths
        const std::string pathStr = filepath.string();

        if (const auto view = IO::VFS::ReadVirtualView(pathStr)) {
            dataView = *view;
        } else if (auto owned = IO::VFS::ReadVirtual(pathStr)) {
            ownedData = std::move(*owned);
            dataView = ownedData;
        } else if (IO::VFS::IsProjectLoaded()) {
            if (auto loosePath = IO::VFS::GetProjectRoot() / filepath; std::filesystem::exists(loosePath)) {
                if (std::ifstream file(loosePath); file.is_open()) {
                    ownedData.assign(std::istreambuf_iterator(file), std::istreambuf_iterator<char>());
                    dataView = ownedData;
                }
            }
        }

        if (dataView.empty()) {
            LOG_WARN(LOG_WHO, "Graphics config not found: " + pathStr + ". Using defaults");
            return config;
        }

        try {
            nlohmann::json j = nlohmann::json::parse(dataView);

            GraphicsConfig parsed;

            if (j.contains("window")) {
                auto &w = j["window"];
                if (w.contains("width"))
                    parsed.WindowWidth = w["width"];
                if (w.contains("height"))
                    parsed.WindowHeight = w["height"];
                if (w.contains("fullscreen"))
                    parsed.Fullscreen = w["fullscreen"];
            }
            if (j.contains("antialiasing")) {
                auto &aa = j["antialiasing"];
                if (aa.contains("MSAA"))
                    parsed.MSAAEnabled = aa["MSAA"];
                if (aa.contains("samples"))
                    parsed.AASamples = aa["samples"];
            }
            if (j.contains("targetfps"))
                parsed.TargetFPS = j["targetfps"];
            if (j.contains("vsync")) {
                if (j["vsync"].is_string())
                    parsed.VSync = StringToVSync(j["vsync"].get<std::string>());
                else if (j["vsync"].is_boolean())
                    parsed.VSync = j["vsync"].get<bool>() ? VSyncType::STANDARD : VSyncType::NONE;
            }

            if (j.contains("overlay")) {
                parsed.ShowPerformanceOverlay = j["overlay"].get<bool>();
            }

            LOG_INFO(LOG_WHO, "Loaded graphics config:");
            LOG_INFO(LOG_WHO, "  Window:        " + std::to_string(parsed.WindowWidth) + "x" + std::to_string(parsed.WindowHeight) + (parsed.Fullscreen ? " (fullscreen)" : ""));
            LOG_INFO(LOG_WHO, "  VSync:         " + std::string(VSyncToString(parsed.VSync)));
            LOG_INFO(LOG_WHO, "  Target FPS:    " + std::to_string(parsed.TargetFPS));
            LOG_INFO(LOG_WHO, "  MSAA:          " + std::string(parsed.MSAAEnabled ? "on (" + std::to_string(parsed.AASamples) + "x)" : "off"));
            LOG_INFO(LOG_WHO, "  FPS Overlay:   " + std::string(parsed.ShowPerformanceOverlay ? "on" : "off"));

            config = std::move(parsed);
        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, "Failed to parse graphics config: " + std::string(e.what()));
        }

        return config;
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
