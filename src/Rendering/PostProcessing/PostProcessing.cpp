#include "PostProcessing.h"
#include "Rendering/Types/Shader/Shader.h"
#include <glad/glad.h>

namespace Rendering::PostProcessing {

    void DrawFullscreenTriangle() {
        static GLuint s_EmptyVAO = 0;
        if (s_EmptyVAO == 0) {
            glGenVertexArrays(1, &s_EmptyVAO);
        }
        glBindVertexArray(s_EmptyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
    }

    FrameBuffer *PostProcessor::Execute(FrameBuffer *scene, FrameBuffer *pingA, FrameBuffer *pingB) {
        FrameBuffer *ping[2] = {pingA, pingB};
        FrameBuffer *currentInput = scene;
        FrameBuffer *lastOutput = nullptr;
        int writeIdx = 0;
        bool ran = false;

        glDisable(GL_BLEND);

        for (auto &fx : m_Effects) {
            if (!fx.enabled || !fx.shader || !fx.shader->IsValid())
                continue;

            FrameBuffer *target = ping[writeIdx];
            target->Bind();
            glDrawBuffer(GL_COLOR_ATTACHMENT0);

            Shader &shader = *fx.shader;
            shader.Bind();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, currentInput->GetColorAttID());

            shader.SetUniform1i("u_Texture", 0);
            shader.SetUniform1f("u_Strength", fx.strength);
            if (fx.bindUniforms)
                fx.bindUniforms(shader);

            DrawFullscreenTriangle();

            currentInput = target;
            lastOutput = target;
            writeIdx ^= 1;
            ran = true;
        }

        glEnable(GL_BLEND);
        return ran ? lastOutput : nullptr;
    }

} // namespace Rendering::PostProcessing
