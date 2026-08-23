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
        if (data.contains("velocityMin") && data["velocityMin"].is_array() && data["velocityMin"].size() >= 3)
            ec.velocityMin = {data["velocityMin"][0], data["velocityMin"][1], data["velocityMin"][2]};
        if (data.contains("velocityMax") && data["velocityMax"].is_array() && data["velocityMax"].size() >= 3)
            ec.velocityMax = {data["velocityMax"][0], data["velocityMax"][1], data["velocityMax"][2]};
        if (data.contains("gravity") && data["gravity"].is_array() && data["gravity"].size() >= 3)
            ec.gravity = {data["gravity"][0], data["gravity"][1], data["gravity"][2]};
        if (data.contains("sizeStartMin"))
            ec.sizeStartMin = data["sizeStartMin"].get<float>();
        else if (data.contains("sizeStart"))
            ec.sizeStartMin = ec.sizeStartMax = data["sizeStart"].get<float>();
        if (data.contains("sizeStartMax"))
            ec.sizeStartMax = data["sizeStartMax"].get<float>();
        if (data.contains("sizeEndMin"))
            ec.sizeEndMin = data["sizeEndMin"].get<float>();
        else if (data.contains("sizeEnd"))
            ec.sizeEndMin = ec.sizeEndMax = data["sizeEnd"].get<float>();
        if (data.contains("sizeEndMax"))
            ec.sizeEndMax = data["sizeEndMax"].get<float>();
        if (data.contains("rotationSpeedMin"))
            ec.rotationSpeedMin = data["rotationSpeedMin"].get<float>();
        if (data.contains("rotationSpeedMax"))
            ec.rotationSpeedMax = data["rotationSpeedMax"].get<float>();
        if (data.contains("colorStart") && data["colorStart"].is_array() && data["colorStart"].size() >= 4)
            ec.colorStart = {data["colorStart"][0], data["colorStart"][1], data["colorStart"][2], data["colorStart"][3]};
        if (data.contains("colorEnd") && data["colorEnd"].is_array() && data["colorEnd"].size() >= 4)
            ec.colorEnd = {data["colorEnd"][0], data["colorEnd"][1], data["colorEnd"][2], data["colorEnd"][3]};
        if (data.contains("isBillboard"))
            ec.isBillboard = data["isBillboard"].get<bool>();
        if (data.contains("blendMode")) {
            const int bm = data["blendMode"].get<int>();
            ec.blendMode = bm == 1 ? ECS::Components::ParticleBlendMode::Additive : ECS::Components::ParticleBlendMode::Alpha;
        }
        if (data.contains("renderOrder"))
            ec.renderOrder = data["renderOrder"].get<int>();
        if (data.contains("shape"))
            ec.shape = data["shape"].get<int>();
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
        data["sizeStartMin"] = ec.sizeStartMin;
        data["sizeStartMax"] = ec.sizeStartMax;
        data["sizeEndMin"] = ec.sizeEndMin;
        data["sizeEndMax"] = ec.sizeEndMax;
        data["rotationSpeedMin"] = ec.rotationSpeedMin;
        data["rotationSpeedMax"] = ec.rotationSpeedMax;
        data["colorStart"] = {ec.colorStart.x, ec.colorStart.y, ec.colorStart.z, ec.colorStart.w};
        data["colorEnd"] = {ec.colorEnd.x, ec.colorEnd.y, ec.colorEnd.z, ec.colorEnd.w};
        data["isBillboard"] = ec.isBillboard;
        data["blendMode"] = ec.blendMode;
        data["renderOrder"] = ec.renderOrder;
        data["shape"] = ec.shape;
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
        if (!file) {
            if (auto *logger = Logging::LoggerService::Get())
                logger->log("ParticleEmitterPrefabManager", "Failed to write emitter preset to: " + filepath, Logging::LogSeverity::Error);
            return false;
        }
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
