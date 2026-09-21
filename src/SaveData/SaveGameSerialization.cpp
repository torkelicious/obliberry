#include "SaveGameSerialization.h"
#include "Core/Project.h"
#include "Core/Utils/PathUtils.h"
#include "Core/Utils/OSFileUtils.h"
#include "IO/VFS/VFS.h"
#include "Logger/LoggerService.h"
#include "SaveData/SaveGameManager.h"
#include "nlohmann/json.hpp"
#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <variant>

#pragma push_macro("LOG_WHO")
#define LOG_WHO "SaveGameSerialization"

namespace {
    using json = nlohmann::json;
    json EncodeVal(const Saves::SaveValue &val) {
        return std::visit(
                [](const auto &stored) -> json {
                    using valueType = std::decay_t<decltype(stored)>;

                    if constexpr (std::is_same_v<valueType, double>) {
                        if (!std::isfinite(stored)) {
                            throw std::runtime_error("Cannot serialize non finite number");
                        }
                    }

                    return stored;
                },
                val);
    }

    std::optional<Saves::SaveValue> DecodeVal(const json &val) {
        if (val.is_boolean()) {
            return val.get<bool>();
        }

        if (val.is_number_integer()) {
            return val.get<std::int64_t>();
        }

        if (val.is_number_unsigned()) {
            const auto num = val.get<std::uint64_t>();
            if (num > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
                return std::nullopt;
            }
            return static_cast<int64_t>(num);
        }

        if (val.is_number_float()) {
            const double num = val.get<double>();
            if (!std::isfinite(num)) {
                return std::nullopt;
            }
            return num;
        }

        if (val.is_string()) {
            return val.get<std::string>();
        }

        return std::nullopt;
    }

    std::optional<std::int64_t> DecodeNonNegativeInt(const json &val) {
        if (val.is_number_integer()) {
            const auto result = val.get<std::int64_t>();

            if (result < 0) {
                return std::nullopt;
            }

            return result;
        }

        if (val.is_number_unsigned()) {
            const auto result = val.get<std::uint64_t>();

            if (result > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
                return std::nullopt;
            }

            return static_cast<std::int64_t>(result);
        }

        return std::nullopt;
    }

    json ToJson(const Saves::SaveData &data) {
        json values = json::object();

        for (const auto &[key, value] : data.values) {
            if (key.empty()) {
                throw std::runtime_error("Save data contains empty key");
            }

            values[key] = EncodeVal(value);
        }
        return json{{"version", data.version}, {"name", data.displayName}, {"current_scene", data.currentScene}, {"created_at", data.createdAtUtc}, {"updated_at", data.updatedAtUtc}, {"values", std::move(values)}};
    }

    std::optional<Saves::SaveData> FromJson(const json &j) {
        if (!j.is_object()) {
            return std::nullopt;
        }

        const auto version = j.find("version");
        const auto name = j.find("name");
        const auto currScene = j.find("current_scene");
        const auto creationTime = j.find("creation_time");
        const auto updatedTime = j.find("updated_time");
        const auto values = j.find("values");

        if (version == j.end() || name == j.end() || currScene == j.end() || creationTime == j.end() || updatedTime == j.end() || values == j.end()) {
            return std::nullopt;
        }

        if (!version->is_number() && !version->is_number_unsigned() || !name->is_string() || !currScene->is_string() || !values->is_object()) {
            return std::nullopt;
        }

        const auto parsedVersion = version->get<uint8_t>();

        if (parsedVersion != Saves::SAVE_FORMAT_VERSION) {
            return std::nullopt;
        }

        const auto parsedCreatedAt = DecodeNonNegativeInt(*creationTime);
        const auto parsedUpdatedAt = DecodeNonNegativeInt(*updatedTime);

        if (!parsedCreatedAt || !parsedUpdatedAt) {
            return std::nullopt;
        }

        Saves::SaveData data;
        data.version = parsedVersion;
        data.displayName = name->get<std::string>();
        data.currentScene = currScene->get<std::string>();
        data.createdAtUtc = *parsedCreatedAt;
        data.updatedAtUtc = *parsedUpdatedAt;

        for (const auto &[key, value] : values->items()) {

            if (key.empty()) {
                return std::nullopt;
            }

            auto decoded = DecodeVal(value);
            if (!decoded) {
                return std::nullopt;
            }

            data.values.emplace(key, std::move(*decoded));
        }
        return data;
    }

} // namespace


namespace Saves::IO {

    // <DataHome>/obliberry/<project uuid>/saves
    std::optional<std::filesystem::path> GenerateSavePath() {
        if (!::IO::VFS::IsProjectLoaded()) {
            return std::nullopt;
        }

        const auto &project = Core::Project::GetActive();
        if (!project) {
            return std::nullopt;
        }

        const auto &uuid = project->GetConfig().UUID;

        std::filesystem::path dataDir = Core::PathUtils::GetDataHome();
        std::filesystem::path obliberryDir = dataDir / "obliberry";
        std::filesystem::path projDir = obliberryDir / uuid / "saves";

        if (projDir.empty()) {
            return std::nullopt;
        }

        return projDir;
    }

    bool WriteAtomic(const std::filesystem::path &path, const SaveData &data) {
        const json j = ToJson(data);
        return (Core::Utils::OSFile::WriteAtomic(path, j.dump(4)));
    }
    std::optional<SaveData> Read(const std::filesystem::path &path) {
        try {
            std::ifstream file(path, std::ios::binary);
            if (!file) {
                LOG_ERROR(LOG_WHO, "Could not open save file: " + path.string());
                return std::nullopt;
            }

            json j;
            file >> j;

            if (file.bad()) {
                LOG_ERROR(LOG_WHO, "Failure while reading save file: " + path.string());
                return std::nullopt;
            }

            auto data = FromJson(j);
            if (!data) {
                LOG_ERROR(LOG_WHO, "Invalid file: " + path.string());
                return std::nullopt;
            }
            return data;

        } catch (std::exception &e) {
            LOG_ERROR(LOG_WHO, "Failed to load file file '" + path.string() + "' : " + e.what());
            return std::nullopt;
        }
    }

} // namespace Saves::IO

#pragma pop_macro("LOG_WHO");
