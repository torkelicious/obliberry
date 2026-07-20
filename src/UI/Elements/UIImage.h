#pragma once
#include "Rendering/Texture.h"
#include "UI/UIElement.h"
#include <memory>
namespace UI {
    class UIImage : public UIElement {
    public:
        void SetImage(const std::shared_ptr<Rendering::Texture> &tex){m_Image = tex;}
        void SetColor(const glm::vec4 color) { m_Color = color; }
        [[nodiscard]] const glm::vec4 &GetColor() const { return m_Color; }
        std::shared_ptr<Rendering::Texture> &GetImage(){return m_Image;}

        void Update() override;
        void Draw(UIRenderer *renderer, glm::vec2 finalPos) override;
    private:
        std::shared_ptr<Rendering::Texture> m_Image;
        glm::vec4 m_Color = {1.0f, 1.0f, 1.0f, 1.0f};
    };
}
