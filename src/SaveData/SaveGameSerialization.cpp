#include "SaveGameSerialization.h"

#include "Core/Project.h"
#include "Core/Utils/OSFileUtils.h"
#include "Core/Utils/PathUtils.h"
#include "IO/VFS/VFS.h"
#include "Logger/LoggerService.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

#pragma push_macro("LOG_WHO")
#define LOG_WHO "SaveGameSerialization"

namespace {
    using json = nlohmann::json;

    json EncodeValue(const Saves::SaveValue &value) {
        return std::visit(
                [](const auto &stored) -> json {
                    using ValueType = std::decay_t<decltype(stored)>;

                    if constexpr (std::is_same_v<ValueType, double>) {
                        if (!std::isfinite(stored)) {
                            throw std::runtime_error("Cannot serialize a non-finite number");
                        }
                    }

                    return stored;
                },
                value);
    }

    std::optional<Saves::SaveValue> DecodeValue(const json &value) {
        if (value.is_boolean()) {
            return value.get<bool>();
        }

        if (value.is_number_unsigned()) {
            const auto number = value.get<std::uint64_t>();
            if (number > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
                return std::nullopt;
            }
            return static_cast<std::int64_t>(number);
        }

        if (value.is_number_integer()) {
            return value.get<std::int64_t>();
        }

        if (value.is_number_float()) {
            const double number = value.get<double>();
            if (!std::isfinite(number)) {
                return std::nullopt;
            }
            return number;
        }

        if (value.is_string()) {
            return value.get<std::string>();
        }

        return std::nullopt;
    }

    std::optional<std::int64_t> DecodeNonNegativeInteger(const json &value) {
        if (value.is_number_unsigned()) {
            const auto number = value.get<std::uint64_t>();
            if (number > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
                return std::nullopt;
            }
            return static_cast<std::int64_t>(number);
        }

        if (value.is_number_integer()) {
            const auto number = value.get<std::int64_t>();
            if (number >= 0) {
                return number;
            }
        }

        return std::nullopt;
    }

    std::optional<std::uint8_t> DecodeVersion(const json &value) {
        if (value.is_number_unsigned()) {
            const auto number = value.get<std::uint64_t>();
            if (number <= std::numeric_limits<std::uint8_t>::max()) {
                return static_cast<std::uint8_t>(number);
            }
            return std::nullopt;
        }

        if (value.is_number_integer()) {
            const auto number = value.get<std::int64_t>();
            if (number >= 0 && number <= std::numeric_limits<std::uint8_t>::max()) {
                return static_cast<std::uint8_t>(number);
            }
        }

        return std::nullopt;
    }

    json ToJson(const Saves::SaveData &data) {
        json values = json::object();

        for (const auto &[key, value] : data.values) {
            if (key.empty()) {
                throw std::runtime_error("Save data contains an empty key");
            }
            values[key] = EncodeValue(value);
        }

        return {
                {"version", data.version}, {"name", data.displayName}, {"current_scene", data.currentScene}, {"created_at", data.createdAtUtc}, {"updated_at", data.updatedAtUtc}, {"values", std::move(values)},
        };
    }

    std::optional<Saves::SaveData> FromJson(const json &document) {
        if (!document.is_object()) {
            return std::nullopt;
        }

        const auto version = document.find("version");
        const auto name = document.find("name");
        const auto currentScene = document.find("current_scene");
        const auto creationTime = document.find("created_at");
        const auto updatedTime = document.find("updated_at");
        const auto values = document.find("values");

        if (version == document.end() || name == document.end() || currentScene == document.end() || creationTime == document.end() || updatedTime == document.end() || values == document.end()) {
            return std::nullopt;
        }

        if (!name->is_string() || !currentScene->is_string() || !values->is_object()) {
            return std::nullopt;
        }

        const auto parsedVersion = DecodeVersion(*version);
        const auto parsedCreatedAt = DecodeNonNegativeInteger(*creationTime);
        const auto parsedUpdatedAt = DecodeNonNegativeInteger(*updatedTime);

        if (!parsedVersion || *parsedVersion != Saves::SAVE_FORMAT_VERSION || !parsedCreatedAt || !parsedUpdatedAt) {
            return std::nullopt;
        }

        Saves::SaveData data;
        data.version = *parsedVersion;
        data.displayName = name->get<std::string>();
        data.currentScene = currentScene->get<std::string>();
        data.createdAtUtc = *parsedCreatedAt;
        data.updatedAtUtc = *parsedUpdatedAt;

        for (const auto &[key, value] : values->items()) {
            if (key.empty()) {
                return std::nullopt;
            }

            auto decoded = DecodeValue(value);
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
    std::optional<std::filesystem::path> GenerateSaveDirectory() {
        if (!::IO::VFS::IsProjectLoaded()) {
            return std::nullopt;
        }

        const auto &project = Core::Project::GetActive();
        if (!project) {
            return std::nullopt;
        }

        const std::string &uuid = project->GetConfig().UUID;
        const std::filesystem::path projectId(uuid);

        if (projectId.empty() || projectId.is_absolute() || projectId.has_root_path() || projectId.has_parent_path() || projectId != projectId.filename()) {
            LOG_ERROR(LOG_WHO, "The active project has an invalid UUID");
            return std::nullopt;
        }

        try {
            return Core::PathUtils::GetDataHome() / "obliberry" / projectId / "saves";
        } catch (const std::exception &error) {
            LOG_ERROR(LOG_WHO, std::string("Could not determine the save directory: ") + error.what());
            return std::nullopt;
        }
    }

    bool WriteAtomic(const std::filesystem::path &path, const SaveData &data) {
        try {
            return Core::Utils::OSFile::WriteAtomic(path, ToJson(data).dump(4));
        } catch (const std::exception &error) {
            LOG_ERROR(LOG_WHO, "Could not serialize save file '" + path.string() + "': " + error.what());
            return false;
        }
    }

    std::optional<SaveData> Read(const std::filesystem::path &path) {
        try {
            std::ifstream file(path, std::ios::binary);
            if (!file) {
                LOG_ERROR(LOG_WHO, "Could not open save file: " + path.string());
                return std::nullopt;
            }

            json document;
            file >> document;

            auto data = FromJson(document);
            if (!data) {
                LOG_ERROR(LOG_WHO, "Invalid save file: " + path.string());
                return std::nullopt;
            }

            return data;
        } catch (const std::exception &error) {
            LOG_ERROR(LOG_WHO, "Failed to load save file '" + path.string() + "': " + error.what());
            return std::nullopt;
        }
    }

} // namespace Saves::IO

#pragma pop_macro("LOG_WHO")
