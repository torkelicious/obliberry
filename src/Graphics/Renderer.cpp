#include "Renderer.h"

void Renderer::Draw(const Mesh &mesh, Shader &shader, const Transform &transform, const glm::mat4 &vp) {
    shader.Bind();
    glm::mat4 mvp = vp * TransformToMatrix(transform);
    shader.SetUniformMat4("u_MVP", mvp); // now the shader actually gets it

    mesh.Bind();
    glDrawElements(GL_TRIANGLES, mesh.GetIndexCount(), GL_UNSIGNED_INT, nullptr);
}

void Renderer::Clear() {
    glClear(GL_COLOR_BUFFER_BIT);
}
