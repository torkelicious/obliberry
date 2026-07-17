#pragma once
#include "Rendering/Texture.h"
#include "UI/UIElement.h"
#include <memory>
namespace UI {
    class UIImage : public UIElement {
        void Update() override;
        void Draw() override;
    private:
        std::shared_ptr<Rendering::Texture> Image;
    };
}
