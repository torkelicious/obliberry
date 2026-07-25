#pragma once
#include <string>
#include <filesystem>

namespace IO::Package::Tools {
    static std::filesystem::path GetInternalsDirectory() {
        const std::filesystem::path path = std::filesystem::current_path() / "internal";
        return path;
    }

    void PackageCurrentProject(const std::string &outputdir);

    void ExportGame(const std::string &output_dir);
} // namespace IO::Package::Tools
