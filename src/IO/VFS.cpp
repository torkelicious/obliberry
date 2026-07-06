#include "VFS.h"

#include <fstream>
#include <iostream>

#include "Package/Container.h"

namespace IO::VFS {
    struct VFSStorage {
        std::filesystem::path rootDir;
        std::filesystem::path assetsDir;
        bool isLoaded = false;
        bool isPackaged = false;
        ContainerReader packReader;
    };

    static VFSStorage s_State;

    std::filesystem::path GetHomeDirectory() {
#ifdef _WIN32
        const char *path = std::getenv("USERPROFILE");
#else
        const char *path = std::getenv("HOME");
#endif
        return path ? std::filesystem::path(path) : std::filesystem::path{};
    }

    void MountProject(const std::filesystem::path &projectConfigPath) {
        try {
            // Absolute turns a potential relative "project.json" into its true OS representation
            const auto absolutePath = std::filesystem::absolute(projectConfigPath);
            s_State.rootDir = absolutePath.parent_path();
            s_State.assetsDir = s_State.rootDir / "assets";
            s_State.isLoaded = true;

            std::cout << "[VFS] Project mounted successfully at: " << s_State.rootDir.string() << "\n";
        } catch (const std::exception &e) {
            std::cerr << "[VFS] Failed to mount project directory: " << e.what() << "\n";
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
            std::cerr << "[VFS] Failed to open package: " << packagepath.string() << "\n";
            return;
        }
        s_State.isPackaged = true;
        s_State.isLoaded = true;
        std::cout << "[VFS] Package mounted: " << packagepath.string() << "\n";
    }

    bool IsPackaged() {
        return s_State.isPackaged;
    }

    std::optional<std::string> ReadVirtual(const std::filesystem::path &virtualPath) {
        if (s_State.isPackaged) {
            return s_State.packReader.read(virtualPath.generic_string());
        }
        std::ifstream file(Resolve(virtualPath), std::ios::binary);
        if (!file.is_open()) return std::nullopt;
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    std::filesystem::path Resolve(const std::filesystem::path &virtualPath) {
        if (!s_State.isLoaded) {
            // fallback to the binary folder if no project context is provided yet
            return std::filesystem::current_path() / virtualPath;
        }
        return s_State.rootDir / virtualPath;
    }

    std::filesystem::path GetProjectRoot() { return s_State.rootDir; }
    std::filesystem::path GetAssetsDirectory() { return s_State.assetsDir; }
    bool IsProjectLoaded() { return s_State.isLoaded; }
} // namespace IO::VFS
