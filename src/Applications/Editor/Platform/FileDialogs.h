#pragma once
#include <optional>
#include <string>

namespace Core {
    struct EngineContext;
}

namespace Editor::Platform {
    struct FileDialogOptions {
        const char *filterName = nullptr;
        const char *filterExt = nullptr;
        const char *defaultPath = nullptr;
        const char *defaultName = nullptr;
        const char *title = nullptr;
        const char *acceptBtnLabel = nullptr;
        const char *cancelBtnLabel = nullptr;
    };

    class FileDialogs {
    public:
        // these return absolute paths
        static std::optional<std::string> OpenFile(const Core::EngineContext &ctx, const FileDialogOptions &options = {});

        static std::optional<std::string> SaveFile(const Core::EngineContext &ctx, const FileDialogOptions &options = {});

        static std::optional<std::string> PickFolder(const Core::EngineContext &ctx, const char *defaultPath = nullptr, const char *title = nullptr, const char *acceptBtnLabel = nullptr,
                                                     const char *cancelBtnLabel = nullptr);
    };
} // namespace Editor::Platform
