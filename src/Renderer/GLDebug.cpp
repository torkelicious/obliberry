#include "GLDebug.h"
#include <iostream>

// I don't remember from where but this code is from somewhere online

void APIENTRY GLDebug::glDebugOutput(const GLenum source, const GLenum type,
                                     const unsigned int id, const GLenum severity,
                                     GLsizei length, const char *message,
                                     const void *userParam) {
    // ignore non-significant error/warning codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
        return;

    std::cout << "---------------" << "\n";
    std::cout << "Debug message (" << id << "): " << message << "\n";

    switch (source) {
        case GL_DEBUG_SOURCE_API:
            std::cout << "Source: API";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            std::cout << "Source: Window System";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            std::cout << "Source: Shader Compiler";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            std::cout << "Source: Third Party";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            std::cout << "Source: Application";
            break;
        case GL_DEBUG_SOURCE_OTHER:
            std::cout << "Source: Other";
            break;
    }
    std::cout << "\n";

    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            std::cout << "Type: Error";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            std::cout << "Type: Deprecated Behaviour";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            std::cout << "Type: Undefined Behaviour";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            std::cout << "Type: Portability";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            std::cout << "Type: Performance";
            break;
        case GL_DEBUG_TYPE_MARKER:
            std::cout << "Type: Marker";
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            std::cout << "Type: Push Group";
            break;
        case GL_DEBUG_TYPE_POP_GROUP:
            std::cout << "Type: Pop Group";
            break;
        case GL_DEBUG_TYPE_OTHER:
            std::cout << "Type: Other";
            break;
    }
    std::cout << "\n";

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            std::cout << "Severity: high";
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            std::cout << "Severity: medium";
            break;
        case GL_DEBUG_SEVERITY_LOW:
            std::cout << "Severity: low";
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            std::cout << "Severity: notification";
            break;
    }
    std::cout << "\n";
    std::cout << "\n";
}


int GLDebug::InitDebug() {
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << "\n";
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << "\n";
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << "\n";
    std::cout << "\n";

    if (!glDebugMessageCallback) {
        std::cerr << "debug not available (glDebugMessageCallback == NULL)\n";
        return -1;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    glDebugMessageCallback(glDebugOutput, nullptr);

    glDebugMessageControl(
        GL_DONT_CARE,
        GL_DONT_CARE,
        GL_DONT_CARE,
        0,
        nullptr,
        GL_TRUE
    );

    return 0;
}

VRAMStats GLDebug::GetVRAMStats() {
    VRAMStats stats;
    const GLubyte *renderer = glGetString(GL_RENDERER);
    if (!renderer) return stats;
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
