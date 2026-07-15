#include "FileDialogs.h"
#include "Core/EngineContext.h"
#include "Platform/Window/Window.h"

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

namespace Editor::Platform {
    static nfdwindowhandle_t GetNativeHandle(const Core::EngineContext &ctx) {
        nfdwindowhandle_t nativewin = {};
        if (ctx.window && ctx.window->GetNativeWindow()) {
            NFD_GetNativeWindowFromGLFWWindow(ctx.window->GetNativeWindow(), &nativewin);
        }
        return nativewin;
    }

    std::optional<std::string> FileDialogs::OpenFile(const Core::EngineContext &ctx, const FileDialogOptions &options) {
        const nfdfilteritem_t filterItem[1] = {{options.filterName, options.filterExt}};

        nfdopendialogu8args_t args = {nullptr};
        args.filterList = options.filterName && options.filterExt ? filterItem : nullptr;
        args.filterCount = options.filterName && options.filterExt ? 1 : 0;
        args.defaultPath = options.defaultPath;
        args.parentWindow = GetNativeHandle(ctx);
        args.title = options.title;
        args.acceptLabel = options.acceptBtnLabel;
        args.cancelLabel = options.cancelBtnLabel;

        nfdnchar_t *outPathRaw = nullptr;
        if (NFD_OpenDialogU8_With(&outPathRaw, &args) == NFD_OKAY && outPathRaw) {
            const NFD::UniquePathU8 outPath(outPathRaw);
            return std::string(outPath.get());
        }
        return std::nullopt;
    }

    std::optional<std::string> FileDialogs::SaveFile(const Core::EngineContext &ctx, const FileDialogOptions &options) {
        const nfdfilteritem_t filterItem[1] = {{options.filterName, options.filterExt}};

        nfdsavedialogu8args_t args = {nullptr};
        args.filterList = options.filterName && options.filterExt ? filterItem : nullptr;
        args.filterCount = options.filterName && options.filterExt ? 1 : 0;
        args.defaultPath = options.defaultPath;
        args.defaultName = options.defaultName;
        args.parentWindow = GetNativeHandle(ctx);
        args.title = options.title;
        args.acceptLabel = options.acceptBtnLabel;
        args.cancelLabel = options.cancelBtnLabel;

        nfdnchar_t *outPathRaw = nullptr;
        if (NFD_SaveDialogU8_With(&outPathRaw, &args) == NFD_OKAY && outPathRaw) {
            const NFD::UniquePathU8 outPath(outPathRaw);
            return std::string(outPath.get());
        }
        return std::nullopt;
    }

    std::optional<std::string> FileDialogs::PickFolder(const Core::EngineContext &ctx, const char *defaultPath, const char *title, const char *acceptBtnLabel, const char *cancelBtnLabel) {
        nfdpickfolderu8args_t args = {nullptr};
        args.defaultPath = defaultPath;
        args.parentWindow = GetNativeHandle(ctx);
        args.title = title;
        args.acceptLabel = acceptBtnLabel;
        args.cancelLabel = cancelBtnLabel;

        nfdnchar_t *outPathRaw = nullptr;
        if (NFD_PickFolderU8_With(&outPathRaw, &args) == NFD_OKAY && outPathRaw) {
            const NFD::UniquePathU8 outPath(outPathRaw);
            return std::string(outPath.get());
        }
        return std::nullopt;
    }
} // namespace Editor::Platform
