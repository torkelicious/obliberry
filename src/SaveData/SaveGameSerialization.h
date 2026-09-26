#pragma once

#include "Config/ProjectConfig.h"
#include "SaveData/SaveGameData.h"

#include <filesystem>
#include <optional>
#include <string_view>

namespace Saves::IO {

    [[nodiscard]] std::optional<std::filesystem::path> GenerateSaveDirectory(std::string_view uuid, Config::SaveLocation location);

    bool WriteAtomic(const std::filesystem::path &path, const SaveData &data);

    [[nodiscard]] std::optional<SaveData> Read(const std::filesystem::path &path);

} // namespace Saves::IO
