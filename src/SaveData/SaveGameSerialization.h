#pragma once

#include "SaveData/SaveGameManager.h"
#include <filesystem>
#include <optional>
namespace Saves::IO {
    std::optional<std::filesystem::path> GenerateSaveDirectory();
    bool WriteAtomic(const std::filesystem::path &path, const SaveData &data);
    std::optional<SaveData> Read(const std::filesystem::path);

} // namespace Saves::IO
