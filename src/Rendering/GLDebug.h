#pragma once


#include <glad/glad.h>

// nvidia memory extension
#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX    0x9048
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX  0x9049

// AMD memory extension
#define GL_VBO_FREE_MEMORY_ATI                           0x87FB

namespace Rendering {
    struct VRAMStats {
        float usedMB = 0.0f;
        float totalMB = 0.0f;
        bool isSupported = false;
    };


    class GLDebug {
    public:
        static void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id,
                                           GLenum severity, GLsizei length,
                                           const char *message, const void *userParam);

        static int InitDebug();

        static VRAMStats GetVRAMStats();
    };
} // namespace Rendering

