#pragma once

#include <algorithm>
#include <exception>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <system_error>

#include "Core/Utils/UUID.h"
#include "Logger/LoggerService.h"

#ifdef _WIN32
#include <windows.h>
#endif

#pragma push_macro("LOG_WHO")
#define LOG_WHO "OSFileUtils"

namespace Core::Utils::OSFile {

    inline bool WriteAtomic(const std::filesystem::path &path, std::string_view data, const std::filesystem::path &tempDir = {}) {
        std::filesystem::path temp;

        try {

            std::filesystem::path candidate = tempDir.empty() ? path : tempDir / path.filename();
            candidate += "." + UUID::UUIDGenerator::Generate() + ".tmp";
            temp = std::move(candidate);

            std::ofstream file;
            file.exceptions(std::ios::failbit | std::ios::badbit);
            file.open(temp, std::ios::binary | std::ios::trunc);

            file << data;

            file.flush();
            file.close();

#ifdef _WIN32
            if (!MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
                const DWORD err = GetLastError();
                throw std::system_error(static_cast<int>(err), std::system_category(), "Failed to replace file");
            }
#else
            std::filesystem::rename(temp, path);
#endif
            return true;
        } catch (const std::exception &e) {
            std::error_code removeError;
            if (!temp.empty()) {
                std::filesystem::remove(temp, removeError);
            }
            LOG_ERROR(LOG_WHO, "Failed to write file '" + path.string() + "': " + e.what());
            return false;
        }
    }

} // namespace Core::Utils::OSFile

#pragma pop_macro("LOG_WHO")
