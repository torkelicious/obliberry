#include "VFS.h"

#include <iostream>

namespace IO::VFS {
    struct VFSStorage {
        std::filesystem::path rootDir;
        std::filesystem::path assetsDir;
        bool isLoaded = false;
    };

    static VFSStorage s_State;

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
}
