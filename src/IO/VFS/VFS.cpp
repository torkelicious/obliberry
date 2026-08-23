#include "VFS.h"
#include <fstream>
#include <sstream>
#include "Logger/LoggerService.h"
#include "IO/Package/Container.h"

#pragma push_macro("LOG_WHO")
#define LOG_WHO "VFS"

namespace IO::VFS {

    struct VFSStorage {
        std::filesystem::path rootDir = {};
        std::filesystem::path assetsDir = {};
        bool isLoaded = false;
        bool isPackaged = false;
        ContainerReader packReader;
    };

    static VFSStorage s_State;

    void MountProject(const std::filesystem::path &projectConfigPath) {
        try {
            // Absolute turns a potential relative "project.json" into its true OS representation
            const auto absolutePath = std::filesystem::absolute(projectConfigPath);
            s_State.rootDir = absolutePath.parent_path();
            s_State.assetsDir = s_State.rootDir / "assets";
            s_State.isLoaded = true;

            LOG_INFO(LOG_WHO, "Project mounted successfully at: " + s_State.rootDir.string());
        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, "Failed to mount project directory: " + std::string(e.what()));
            s_State.isLoaded = false;
        }
    }

    void UnmountProject() {
        s_State.rootDir.clear();
        s_State.assetsDir.clear();
        s_State.isLoaded = false;
    }

    void MountPackage(const std::filesystem::path &packagepath) {
        if (!s_State.packReader.open(packagepath)) {
            LOG_ERROR(LOG_WHO, "Failed to open package: " + packagepath.string());
            return;
        }
        s_State.isPackaged = true;
        s_State.isLoaded = true;
        LOG_INFO(LOG_WHO, "Package mounted: " + packagepath.string());
    }

    bool IsPackaged() { return s_State.isPackaged; }

    std::optional<std::string> ReadVirtual(const std::filesystem::path &virtualPath) {
        if (s_State.isPackaged) {
            return s_State.packReader.read(virtualPath.generic_string());
        }
        std::ifstream file(Resolve(virtualPath), std::ios::binary);
        if (!file.is_open())
            return std::nullopt;
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    std::optional<std::string_view> ReadVirtualView(const std::filesystem::path &virtualPath) {
        if (s_State.isPackaged) {
            return s_State.packReader.read_view(virtualPath.generic_string());
        }
        return std::nullopt;
    }

    std::filesystem::path Resolve(const std::filesystem::path &virtualPath) {
        const std::filesystem::path basePath = s_State.isLoaded ? s_State.rootDir : std::filesystem::current_path();

        if (virtualPath.is_absolute()) {
            LOG_ERROR(LOG_WHO, "path was absolute, it will not resolve.");
            return {};
        }

        // normalize paths
        std::filesystem::path combinedPath = basePath / virtualPath;
        std::filesystem::path resolvedPath = std::filesystem::weakly_canonical(combinedPath);
        std::filesystem::path canonicalBase = std::filesystem::weakly_canonical(basePath);

        // verify
        auto [baseIt, resIt] = std::mismatch(canonicalBase.begin(), canonicalBase.end(), resolvedPath.begin());
        if (baseIt != canonicalBase.end()) {
            // path escapes base dir
            LOG_ERROR(LOG_WHO, "path traversal blocked for: " + virtualPath.string() + " (escapes base dir)");
            return {};
        }
        return resolvedPath;
    }

    std::filesystem::path GetProjectRoot() { return s_State.rootDir; }
    std::filesystem::path GetAssetsDirectory() { return s_State.assetsDir; }
    bool IsProjectLoaded() { return s_State.isLoaded; }
} // namespace IO::VFS
#pragma pop_macro("LOG_WHO")
