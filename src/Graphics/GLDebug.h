

#ifndef ISOMETRICGAME_DEBUG_H
#define ISOMETRICGAME_DEBUG_H

#include <glad/glad.h>

class GLDebug {
public:
    static void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id,
                                       GLenum severity, GLsizei length,
                                       const char *message, const void *userParam);

    static int initDbg();
};
#endif //ISOMETRICGAME_DEBUG_H
