#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Rendering {
    class Transform {
    public:
        Transform() = default;

        void SetPosition(const glm::vec3 &position) {
            if (m_UseCustomMatrix) {
                m_CachedMatrix[3] = glm::vec4(position, m_CachedMatrix[3].w);
            }
            m_Position = position;
            m_IsDirty = true;
        }

        void SetRotation(const glm::vec3 &rotation) {
            if (m_UseCustomMatrix && rotation != m_Rotation) {
                const glm::mat3 delta = glm::mat3(RotationMatrix(rotation)) * glm::transpose(glm::mat3(RotationMatrix(m_Rotation)));
                for (int column = 0; column < 3; ++column) {
                    m_CachedMatrix[column] = glm::vec4(delta * glm::vec3(m_CachedMatrix[column]), m_CachedMatrix[column].w);
                }
            }
            m_Rotation = rotation;
            m_IsDirty = true;
        }

        void SetScale(const glm::vec3 &scale) {
            if (m_UseCustomMatrix && scale != m_Scale) {
                const glm::mat4 rotation = RotationMatrix(m_Rotation);
                for (int column = 0; column < 3; ++column) {
                    if (m_Scale[column] != 0.0f) {
                        m_CachedMatrix[column] *= scale[column] / m_Scale[column];
                    } else if (scale[column] != 0.0f) {
                        // Restore a collapsed axis from rotation.
                        m_CachedMatrix[column] = rotation[column] * scale[column];
                    }
                }
            }
            m_Scale = scale;
            m_IsDirty = true;
        }

        void SetCustomMatrix(const glm::mat4 &matrix) {
            m_CachedMatrix = matrix;
            m_UseCustomMatrix = true;
        }

        [[nodiscard]] bool UsesCustomMatrix() const { return m_UseCustomMatrix; }

        void ClearCustomMatrix() {
            m_UseCustomMatrix = false;
            m_IsDirty = true;
        }

        [[nodiscard]] bool IsDirty() const { return m_IsDirty; }

        void ClearDirty() const { m_IsDirty = false; }

        const glm::vec3 &GetPosition() const { return m_Position; }
        const glm::vec3 &GetRotation() const { return m_Rotation; }
        const glm::vec3 &GetScale() const { return m_Scale; }

        const glm::mat4 &GetMatrix() const {
            if (m_UseCustomMatrix) {
                return m_CachedMatrix;
            }

            if (m_IsDirty) {
                UpdateMatrix();
                m_IsDirty = false;
            }
            return m_CachedMatrix;
        }

    private:
        static glm::mat4 RotationMatrix(const glm::vec3 &rotation) {
            glm::mat4 matrix(1.0f);
            if (rotation.x != 0.0f)
                matrix = glm::rotate(matrix, rotation.x, glm::vec3(1, 0, 0));
            if (rotation.y != 0.0f)
                matrix = glm::rotate(matrix, rotation.y, glm::vec3(0, 1, 0));
            if (rotation.z != 0.0f)
                matrix = glm::rotate(matrix, rotation.z, glm::vec3(0, 0, 1));
            return matrix;
        }

        void UpdateMatrix() const {
            const glm::mat4 model = glm::translate(glm::mat4(1.0f), m_Position) * RotationMatrix(m_Rotation);
            m_CachedMatrix = glm::scale(model, m_Scale);
        }

        glm::vec3 m_Position{0.0f};
        glm::vec3 m_Rotation{0.0f};
        glm::vec3 m_Scale{1.0f};

        mutable glm::mat4 m_CachedMatrix{1.0f};
        mutable bool m_IsDirty{true};

        bool m_UseCustomMatrix{false};
    };
} // namespace Rendering
