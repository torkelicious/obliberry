#pragma once
#include "Core/Constants.h"
#include "Core/ResourceManager.h"
#include "Core/Utils/JsonUtils.h"
#include "ECS/Components/ParticleEmitterComponent.h"
#include "IO/VFS/VFS.h"
#include "Logger/LoggerService.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace IO {
    inline ECS::Components::ParticleEmitterComponent DeserializeEmitter(const nlohmann::json &data) {
        ECS::Components::ParticleEmitterComponent ec;
        if (data.contains("maxParticles"))
            ec.maxParticles = data["maxParticles"].get<int>();
        if (data.contains("emitRate"))
            ec.emitRate = data["emitRate"].get<float>();
        if (data.contains("lifetimeMin"))
            ec.lifetimeMin = data["lifetimeMin"].get<float>();
        if (data.contains("lifetimeMax"))
            ec.lifetimeMax = data["lifetimeMax"].get<float>();
        if (data.contains("velocityMin"))
            ec.velocityMin = {data["velocityMin"][0], data["velocityMin"][1], data["velocityMin"][2]};
        if (data.contains("velocityMax"))
            ec.velocityMax = {data["velocityMax"][0], data["velocityMax"][1], data["velocityMax"][2]};
        if (data.contains("gravity"))
            ec.gravity = {data["gravity"][0], data["gravity"][1], data["gravity"][2]};
        if (data.contains("sizeStart"))
            ec.sizeStart = data["sizeStart"].get<float>();
        if (data.contains("sizeEnd"))
            ec.sizeEnd = data["sizeEnd"].get<float>();
        if (data.contains("colorStart"))
            ec.colorStart = {data["colorStart"][0], data["colorStart"][1], data["colorStart"][2], data["colorStart"][3]};
        if (data.contains("colorEnd"))
            ec.colorEnd = {data["colorEnd"][0], data["colorEnd"][1], data["colorEnd"][2], data["colorEnd"][3]};
        if (data.contains("isBillboard"))
            ec.isBillboard = data["isBillboard"].get<bool>();
        if (data.contains("material_id")) {
            const std::string matID = data["material_id"].get<std::string>();
            ec.material = Core::ResourceManager::GetInstance().Get<Rendering::Material>(matID);
        }
        return ec;
    }

    inline bool SerializeEmitter(const ECS::Components::ParticleEmitterComponent &ec, const std::string &filepath) {
        nlohmann::json data;
        data["maxParticles"] = ec.maxParticles;
        data["emitRate"] = ec.emitRate;
        data["lifetimeMin"] = ec.lifetimeMin;
        data["lifetimeMax"] = ec.lifetimeMax;
        data["velocityMin"] = {ec.velocityMin.x, ec.velocityMin.y, ec.velocityMin.z};
        data["velocityMax"] = {ec.velocityMax.x, ec.velocityMax.y, ec.velocityMax.z};
        data["gravity"] = {ec.gravity.x, ec.gravity.y, ec.gravity.z};
        data["sizeStart"] = ec.sizeStart;
        data["sizeEnd"] = ec.sizeEnd;
        data["colorStart"] = {ec.colorStart.x, ec.colorStart.y, ec.colorStart.z, ec.colorStart.w};
        data["colorEnd"] = {ec.colorEnd.x, ec.colorEnd.y, ec.colorEnd.z, ec.colorEnd.w};
        data["isBillboard"] = ec.isBillboard;
        if (ec.material) {
            if (std::string id = Core::ResourceManager::GetInstance().GetKey<Rendering::Material>(ec.material); !id.empty()) {
                data["material_id"] = id;
            }
        }

        Core::Utils::Json::RoundJsonFloats(data, 4);

        const auto resolved = VFS::Resolve(filepath);
        std::ofstream file(resolved);
        if (!file.is_open()) {
            if (auto *logger = Logging::LoggerService::Get())
                logger->log("ParticleEmitterPrefabManager", "Failed to save emitter preset to: " + filepath, Logging::LogSeverity::Error);
            return false;
        }
        file << data.dump(4);
        return true;
    }

    inline std::optional<ECS::Components::ParticleEmitterComponent> LoadEmitterPreset(const std::string &filepath) {
        std::string_view dataView;
        std::string ownedData;
        if (const auto view = VFS::ReadVirtualView(filepath))
            dataView = *view;
        else if (auto owned = VFS::ReadVirtual(filepath))
            ownedData = std::move(*owned), dataView = ownedData;
        else {
            if (auto *logger = Logging::LoggerService::Get())
                logger->log("ParticleEmitterPrefabManager", "Failed to load emitter preset: " + filepath + " (Not found in VFS)", Logging::LogSeverity::Error);
            return std::nullopt;
        }

        nlohmann::json json;
        try {
            if (VFS::IsPackaged()) {
                std::vector<uint8_t> bytes(dataView.begin(), dataView.end());
                json = nlohmann::json::from_msgpack(bytes);
            } else {
                json = nlohmann::json::parse(dataView);
            }
        } catch (const std::exception &e) {
            if (auto *logger = Logging::LoggerService::Get())
                logger->log("ParticleEmitterPrefabManager", "JSON parse error for " + filepath + ": " + e.what(), Logging::LogSeverity::Error);
            return std::nullopt;
        }

        return DeserializeEmitter(json);
    }

    inline std::vector<std::string> GetEmitterPresetFiles() {
        std::vector<std::string> files;
        if (const auto resolved = VFS::Resolve(std::string(Core::PARTICLE_PRESET_PATH)); std::filesystem::exists(resolved)) {
            for (const auto &entry : std::filesystem::directory_iterator(resolved)) {
                if (entry.is_regular_file() && entry.path().extension() == ".json") {
                    auto relPath = std::filesystem::relative(entry.path(), VFS::GetProjectRoot());
                    std::string relStr = relPath.string();
                    for (auto &c : relStr)
                        if (c == '\\')
                            c = '/';
                    files.push_back(std::move(relStr));
                }
            }
        }
        return files;
    }
} // namespace IO
