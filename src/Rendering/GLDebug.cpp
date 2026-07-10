#include "GLDebug.h"
#include "Core/LoggerService.h"

// Heavily based off of;
// https://learnopengl.com/In-Practice/Debugging


constexpr auto LOG_WHO = "GLDebug";

namespace Rendering {
    void APIENTRY GLDebug::glDebugOutput(const GLenum source, const GLenum type, const unsigned int id,
                                         const GLenum severity, GLsizei length, const char *message,
                                         const void *userParam) {
        // ignore non-significant error/warning codes
        if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
            return;

        LOG_INFO(LOG_WHO, std::string("---------------") + "\nDebug message (" + std::to_string(id) + "): " + message);

        // ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
        switch (source) {
            case GL_DEBUG_SOURCE_API:
                LOG_INFO(LOG_WHO, "Source: API");
                break;
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
                LOG_INFO(LOG_WHO, "Source: Window System");
                break;
            case GL_DEBUG_SOURCE_SHADER_COMPILER:
                LOG_INFO(LOG_WHO, "Source: Shader Compiler");
                break;
            case GL_DEBUG_SOURCE_THIRD_PARTY:
                LOG_INFO(LOG_WHO, "Source: Third Party");
                break;
            case GL_DEBUG_SOURCE_APPLICATION:
                LOG_INFO(LOG_WHO, "Source: Application");
                break;
            case GL_DEBUG_SOURCE_OTHER:
                LOG_INFO(LOG_WHO, "Source: Other");
                break;
        }
        std::cout << "\n";

        // ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
        switch (type) {
            case GL_DEBUG_TYPE_ERROR:
                LOG_INFO(LOG_WHO, "Type: Error");
                break;
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
                LOG_INFO(LOG_WHO, "Type: Deprecated Behaviour");
                break;
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
                LOG_INFO(LOG_WHO, "Type: Undefined Behaviour");
                break;
            case GL_DEBUG_TYPE_PORTABILITY:
                LOG_INFO(LOG_WHO, "Type: Portability");
                break;
            case GL_DEBUG_TYPE_PERFORMANCE:
                LOG_INFO(LOG_WHO, "Type: Performance");
                break;
            case GL_DEBUG_TYPE_MARKER:
                LOG_INFO(LOG_WHO, "Type: Marker");
                break;
            case GL_DEBUG_TYPE_PUSH_GROUP:
                LOG_INFO(LOG_WHO, "Type: Push Group");
                break;
            case GL_DEBUG_TYPE_POP_GROUP:
                LOG_INFO(LOG_WHO, "Type: Pop Group");
                break;
            case GL_DEBUG_TYPE_OTHER:
                LOG_INFO(LOG_WHO, "Type: Other");
                break;
        }
        std::cout << "\n";

        // ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:
                LOG_INFO(LOG_WHO, "Severity: high");
                break;
            case GL_DEBUG_SEVERITY_MEDIUM:
                LOG_INFO(LOG_WHO, "Severity: medium");
                break;
            case GL_DEBUG_SEVERITY_LOW:
                LOG_INFO(LOG_WHO, "Severity: low");
                break;
            case GL_DEBUG_SEVERITY_NOTIFICATION:
                LOG_INFO(LOG_WHO, "Severity: notification");
                break;
        }
        std::cout << "\n";
        std::cout << "\n";
    }


    int GLDebug::InitDebug() {
        LOG_INFO(LOG_WHO, "OpenGL Version: " + std::string(reinterpret_cast<const char *>(glGetString(GL_VERSION))));
        LOG_INFO(LOG_WHO, "Renderer: " + std::string(reinterpret_cast<const char *>(glGetString(GL_RENDERER))));
        LOG_INFO(LOG_WHO, "Vendor: " + std::string(reinterpret_cast<const char *>(glGetString(GL_VENDOR))));
        LOG_INFO(LOG_WHO, "");

        if (!glDebugMessageCallback) {
            LOG_ERROR(LOG_WHO, "debug not available (glDebugMessageCallback == NULL)");
            return -1;
        }

        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

        glDebugMessageCallback(glDebugOutput, nullptr);

        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

        return 0;
    }

    VRAMStats GLDebug::GetVRAMStats() {
        VRAMStats stats;
        const GLubyte *renderer = glGetString(GL_RENDERER);
        if (!renderer)
            return stats;
        // NVIDIA Path
        if (const std::string rendererStr(reinterpret_cast<const char *>(renderer));
            rendererStr.find("NVIDIA") != std::string::npos) {
            GLint totalKb = 0;
            GLint currentAvailableKb = 0;
            glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &totalKb);
            glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &currentAvailableKb);

            if (totalKb > 0) {
                stats.totalMB = static_cast<float>(totalKb) / 1024.0f;
                stats.usedMB = static_cast<float>(totalKb - currentAvailableKb) / 1024.0f;
                stats.isSupported = true;
            }
        }
        // AMD only reports free memory blocks via OpenGL
        else if (rendererStr.find("AMD") != std::string::npos || rendererStr.find("ATI") != std::string::npos) {
            GLint freeMemKb[4];
            glGetIntegerv(GL_VBO_FREE_MEMORY_ATI, freeMemKb);

            // freeMemKb[0] is total free memory remaining in the pool
            stats.usedMB = 0.0f;
            stats.totalMB = static_cast<float>(freeMemKb[0]) / 1024.0f; // remaining pool total
            stats.isSupported = true;
        }
        return stats;
    }
} // namespace Rendering
