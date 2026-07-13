#include "GraphicsConfig.h"
#include "LoggerService.h"
#include "IO/VFS.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#pragma push_macro("LOG_WHO")
#define LOG_WHO "GraphicsConfig"

namespace Core::Graphics {

    // VSync JSON string mapping
    static const char *VSyncToString(const VSyncType vsync) {
        switch (vsync) {
            case VSyncType::NONE: return "none";
            case VSyncType::STANDARD: return "standard";
            case VSyncType::ADAPTIVE: return "adaptive";
        }
        return "standard";
    }

    static VSyncType StringToVSync(const std::string &str) {
        if (str == "none") return VSyncType::NONE;
        if (str == "adaptive") return VSyncType::ADAPTIVE;
        return VSyncType::STANDARD;
    }

    GraphicsConfig GraphicsConfig::Deserialize(const std::string &filepath) {
        GraphicsConfig config;
        std::string_view dataView;
        std::string ownedData;
        if (const auto view = IO::VFS::ReadVirtualView(filepath)) {
            dataView = *view;
        } else if (auto owned = IO::VFS::ReadVirtual(filepath)) {
            ownedData = std::move(*owned);
            dataView = ownedData;
        } else if (IO::VFS::IsProjectLoaded()) {
            auto loosePath = IO::VFS::GetProjectRoot() / filepath;
            if (std::filesystem::exists(loosePath)) {
                std::ifstream file(loosePath);
                ownedData.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                dataView = ownedData;
            }
        }
        if (dataView.empty()) {
            LOG_WARN(LOG_WHO, "Graphics config not found: " + filepath + ". Using defaults");
            return config;
        }
        try {
            nlohmann::json j = nlohmann::json::parse(dataView);
            if (j.contains("window")) {
                auto &w = j["window"];
                if (w.contains("width"))
                    config.WindowWidth = w["width"];
                if (w.contains("height"))
                    config.WindowHeight = w["height"];
                if (w.contains("fullscreen"))
                    config.Fullscreen = w["fullscreen"];
            }
            if (j.contains("antialiasing")) {
                auto &aa = j["antialiasing"];
                if (aa.contains("MSAA"))
                    config.MSAAEnabled = aa["MSAA"];
                if (aa.contains("samples"))
                    config.AASamples = aa["samples"];
            }
            if (j.contains("targetfps"))
                config.TargetFPS = j["targetfps"];
            if (j.contains("vsync")) {
                if (j["vsync"].is_string())
                    config.VSync = StringToVSync(j["vsync"].get<std::string>());
                else if (j["vsync"].is_boolean())
                    config.VSync = j["vsync"].get<bool>() ? VSyncType::STANDARD : VSyncType::NONE;
            }

            LOG_INFO(LOG_WHO, "Loaded graphics config:");
            LOG_INFO(LOG_WHO, "  Window:        " + std::to_string(config.WindowWidth) + "x" + std::to_string(config.WindowHeight) + (config.Fullscreen ? " (fullscreen)" : ""));
            LOG_INFO(LOG_WHO, "  VSync:         " + std::string(VSyncToString(config.VSync)));
            LOG_INFO(LOG_WHO, "  Target FPS:    " + std::to_string(static_cast<int>(config.TargetFPS)));
            LOG_INFO(LOG_WHO, "  MSAA:          " + std::string(config.MSAAEnabled ? "on (" + std::to_string(config.AASamples) + "x)" : "off"));
        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, "Failed to parse graphics config: " + std::string(e.what()));
        }

        return config;
    }

    void GraphicsConfig::Serialize(const GraphicsConfig &conf, const std::string &filepath) {
        try {
            nlohmann::json j;
            j["window"]["width"] = conf.WindowWidth;
            j["window"]["height"] = conf.WindowHeight;
            j["window"]["fullscreen"] = conf.Fullscreen;
            j["antialiasing"]["MSAA"] = conf.MSAAEnabled;
            j["antialiasing"]["samples"] = conf.AASamples;
            j["targetfps"] = conf.TargetFPS;
            j["vsync"] = VSyncToString(conf.VSync);

            std::filesystem::path resolvedPath = IO::VFS::Resolve(filepath);
            std::ofstream file(resolvedPath);
            if (!file.is_open()) {
                LOG_ERROR(LOG_WHO, "Failed to open graphics config for writing: " + resolvedPath.string());
                return;
            }
            file << j.dump(2);
        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, "Failed to serialize graphics config: " + std::string(e.what()));
        }
    }

} // namespace Core::Graphics

#pragma pop_macro("LOG_WHO")
