#include "PostProcessing.h"
#include <glad/glad.h>

namespace Rendering::PostProcessing {

    void PostProcessEffect::DrawFullscreenTriangle() {
        static GLuint s_EmptyVAO = 0;
        if (s_EmptyVAO == 0) {
            glGenVertexArrays(1, &s_EmptyVAO);
        }
        glBindVertexArray(s_EmptyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
    }

    FrameBuffer *PostProcessor::Execute(const uint32_t sceneColTex, FrameBuffer *pingA, FrameBuffer *pingB) const {
        FrameBuffer *ping[2] = {pingA, pingB};
        uint32_t currentInput = sceneColTex;
        FrameBuffer *lastOutput = nullptr;
        int writeIdx = 0;
        bool ran = false;

        for (auto &fx : m_Effects) {
            if (!fx->enabled)
                continue;
            FrameBuffer *target = ping[writeIdx];
            fx->Apply(currentInput, target);
            currentInput = target->GetColorAttID();
            lastOutput = target;
            writeIdx ^= 1;
            ran = true;
        }
        return ran ? lastOutput : nullptr;
    }

} // namespace Rendering::PostProcessing
