#pragma once
#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace IO::VFS {
    // the active project root is based on the path of project.json
    void MountProject(const std::filesystem::path &projectConfigPath);

    void UnmountProject();

    void MountPackage(const std::filesystem::path &packagepath);

    bool IsPackaged();

    std::optional<std::string> ReadVirtual(const std::filesystem::path &virtualPath);

    // returns string_view into mmap blob.
    // returns nullopt if not packaged, entry is compressed, or not found
    std::optional<std::string_view> ReadVirtualView(const std::filesystem::path &virtualPath);

    [[nodiscard]] std::filesystem::path Resolve(const std::filesystem::path &virtualPath);

    [[nodiscard]] std::filesystem::path GetProjectRoot();

    [[nodiscard]] std::filesystem::path GetAssetsDirectory();

    [[nodiscard]] bool IsProjectLoaded();

    [[nodiscard]] inline std::string ToRelative(const std::filesystem::path &inputPath) {
        // already relative
        if (!inputPath.is_absolute()) {
            std::string result = inputPath.string();
            std::ranges::replace(result, '\\', '/');
            return result;
        }
        const auto root = GetProjectRoot();
        if (root.empty())
            return inputPath.string();
        const auto rel = std::filesystem::relative(inputPath, root);
        std::string result = rel.string();
        std::ranges::replace(result, '\\', '/');
        return result;
    }
} // namespace IO::VFS
