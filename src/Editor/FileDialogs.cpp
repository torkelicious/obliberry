#include "FileDialogs.h"
#include "Core/EngineContext.h"
#include "Core/Window.h"
#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_WAYLAND
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <nfd.hpp>
#include <nfd_glfw3.h>

namespace Editor {
    static nfdwindowhandle_t GetNativeHandle(const Core::EngineContext &ctx) {
        nfdwindowhandle_t nativewin;
        NFD_GetNativeWindowFromGLFWWindow(ctx.window->GetNativeWindow(), &nativewin);
        return nativewin;
    }

    std::optional<std::string> FileDialogs::OpenFile(const Core::EngineContext &ctx,
                                                     const char *filterName,
                                                     const char *filterExt, const char *defaultPath) {
        const nfdfilteritem_t filterItem[1] = {{filterName, filterExt}};
        NFD::UniquePath outPath;

        if (NFD::OpenDialog(outPath, filterItem, 1, defaultPath, GetNativeHandle(ctx)) == NFD_OKAY) {
            return std::string(outPath.get());
        }
        return std::nullopt;
    }

    std::optional<std::string> FileDialogs::SaveFile(const Core::EngineContext &ctx,
                                                     const char *filterName,
                                                     const char *filterExt, const char *defaultPath) {
        const nfdfilteritem_t filterItem[1] = {{filterName, filterExt}};
        NFD::UniquePath outPath;

        if (NFD::SaveDialog(outPath, filterItem, 1, defaultPath, nullptr, GetNativeHandle(ctx)) == NFD_OKAY) {
            return std::string(outPath.get());
        }
        return std::nullopt;
    }

    std::optional<std::string> FileDialogs::PickFolder(const Core::EngineContext &ctx, const char *defaultPath) {
        NFD::UniquePath outPath;

        if (NFD::PickFolder(outPath, defaultPath, GetNativeHandle(ctx)) == NFD_OKAY) {
            return std::string(outPath.get());
        }
        return std::nullopt;
    }
} // Editor
