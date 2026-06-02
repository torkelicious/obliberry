#include "Renderer.h"

void Renderer::Draw(const Mesh& mesh, Shader& shader, const Transform& transform) {
    shader.Bind();
    glm::mat4 model = TransformToMatrix(transform);
    shader.SetUniformMat4("u_Model", model);

    mesh.Bind();
    glDrawElements(GL_TRIANGLES, mesh.GetIndexCount(), GL_UNSIGNED_INT, nullptr);
}