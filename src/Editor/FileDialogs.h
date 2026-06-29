#pragma once
#include <optional>
#include <string>

namespace Core {
    struct EngineContext;
}

namespace Editor {
    class FileDialogs {
    public:
        // these return absolute paths
        static std::optional<std::string> OpenFile(const Core::EngineContext &ctx,
                                                   const char *filterName,
                                                   const char *filterExt, const char *defaultPath = nullptr);

        static std::optional<std::string> SaveFile(const Core::EngineContext &ctx,
                                                   const char *filterName,
                                                   const char *filterExt, const char *defaultPath = nullptr);

        static std::optional<std::string> PickFolder(const Core::EngineContext &ctx,
                                                     const char *defaultPath = nullptr);
    };
} // Editor
