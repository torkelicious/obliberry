#pragma once
#include <filesystem>
#include <string>

namespace IO::VFS {
    // the active project root is based on the path of project.json
    void MountProject(const std::filesystem::path &projectConfigPath);

    void UnmountProject();

    [[nodiscard]] std::filesystem::path Resolve(const std::filesystem::path &virtualPath);

    [[nodiscard]] std::filesystem::path GetProjectRoot();

    [[nodiscard]] std::filesystem::path GetAssetsDirectory();

    [[nodiscard]] bool IsProjectLoaded();

    [[nodiscard]] std::filesystem::path GetHomeDirectory();
}
