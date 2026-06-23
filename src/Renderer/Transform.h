#pragma once


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Transform {
public:
    Transform() = default;

    void SetPosition(const glm::vec3 &position) {
        m_Position = position;
        m_IsDirty = true;
        m_UseCustomMatrix = false;
    }

    void SetRotation(const glm::vec3 &rotation) {
        m_Rotation = rotation;
        m_IsDirty = true;
        m_UseCustomMatrix = false;
    }

    void SetScale(const glm::vec3 &scale) {
        m_Scale = scale;
        m_IsDirty = true;
        m_UseCustomMatrix = false;
    }

    // overrides TRS calculation
    void SetCustomMatrix(const glm::mat4 &matrix) {
        m_CachedMatrix = matrix;
        m_UseCustomMatrix = true;
        m_IsDirty = false;
    }

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
    void UpdateMatrix() const {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), m_Position);

        if (m_Rotation.x != 0.0f) model = glm::rotate(model, m_Rotation.x, glm::vec3(1, 0, 0));
        if (m_Rotation.y != 0.0f) model = glm::rotate(model, m_Rotation.y, glm::vec3(0, 1, 0));
        if (m_Rotation.z != 0.0f) model = glm::rotate(model, m_Rotation.z, glm::vec3(0, 0, 1));

        m_CachedMatrix = glm::scale(model, m_Scale);
    }

private:
    glm::vec3 m_Position{0.0f};
    glm::vec3 m_Rotation{0.0f};
    glm::vec3 m_Scale{1.0f};

    // note2remember: mutable allows member to be changed inside a const function
    mutable glm::mat4 m_CachedMatrix{1.0f};
    mutable bool m_IsDirty{true};

    bool m_UseCustomMatrix{false};
};

