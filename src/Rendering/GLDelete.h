#pragma once

#include <glad/glad.h>

namespace Rendering {
    void QDeleteTexture(GLuint textureID);
    void QDeleteProgram(GLuint programID);
    void QDeleteBuffer(GLuint buffID);
    void QDeleteVertexArray(GLuint arrID);
    void QDeleteFrameBuffer(GLuint framebufferID, GLuint colorTexID, GLuint entityTexID);
} // namespace Rendering
