#include "UISerializer.h"
#include "Logger/LoggerService.h"
#include "UI/Rendering/UISystem.h"
#include "UI/UIElement.h"
#include "UI/Elements/UIText.h"
#include "UI/Elements/UIButton.h"
#include "UI/Elements/UIImage.h"
#include "UI/Elements/UIRect.h"
#include "UI/Text/Font.h"
#include "Core/ResourceManager.h"

#pragma push_macro("LOG_WHO")
#define LOG_WHO "UISerializer"

namespace IO::UISerializer {
    using json = nlohmann::json;

    static void SerializeRect(json &j, const UI::UIElement *element) {
        j["rect"]["position"] = {element->Rect.Position.x, element->Rect.Position.y};
        j["rect"]["scale"] = {element->Rect.Scale.x, element->Rect.Scale.y};
    }

    static void SerializeFlags(json &j, const UI::UIElement *element) { j["flags"] = element->HasFlag(UI::VISIBLE) | (element->HasFlag(UI::ENABLED) ? 2 : 0); }

    static void SerializeColor(json &j, const char *key, const glm::vec4 &color) { j[key] = {color.r, color.g, color.b, color.a}; }

    static void SerializeElement(nlohmann::json &j, const UI::UIElement *element, Core::ResourceManager &resources) {
        j["name"] = element->Name;
        SerializeRect(j, element);
        SerializeFlags(j, element);

        if (const auto *text = dynamic_cast<const UI::UIText *>(element)) {
            j["type"] = "Text";
            j["text"] = text->GetText();
            SerializeColor(j, "color", text->GetColor());
            if (text->GetFont()) {
                j["font"] = resources.GetKey(text->GetFont());
            }
        } else if (const auto *button = dynamic_cast<const UI::UIButton *>(element)) {
            j["type"] = "Button";
            j["text"] = button->GetText();
            SerializeColor(j, "color", button->GetColor());
            SerializeColor(j, "bg_color", button->GetBackgroundColor());
            SerializeColor(j, "hovered_bg_color", button->GetHoveredBackgroundColor());
            if (button->GetBackgroundTexture()) {
                j["bg_texture"] = resources.GetKey(button->GetBackgroundTexture());
            }
            if (button->GetFont()) {
                j["font"] = resources.GetKey(button->GetFont());
            }
        } else if (const auto *image = dynamic_cast<const UI::UIImage *>(element)) {
            j["type"] = "Image";
            if (const auto &img = image->GetImage()) {
                j["texture"] = resources.GetKey(img);
            }
            SerializeColor(j, "color", image->GetColor());
        } else if (dynamic_cast<const UI::UIRect *>(element)) {
            j["type"] = "Rect";
            SerializeColor(j, "color", static_cast<const UI::UIRect *>(element)->GetColor());
        } else {
            j["type"] = "Element";
        }

        // children
        if (!element->Children.empty()) {
            j["children"] = json::array();
            for (const auto *child : element->Children) {
                json childJson;
                SerializeElement(childJson, child, resources);
                j["children"].push_back(std::move(childJson));
            }
        }
    }

    static void DeserializeRect(const json &j, UI::UIElement *element) {
        if (j.contains("rect")) {
            auto &r = j["rect"];
            if (r.contains("position") && r["position"].is_array() && r["position"].size() >= 2) {
                element->Rect.Position = {r["position"][0].get<float>(), r["position"][1].get<float>()};
            }
            if (r.contains("scale") && r["scale"].is_array() && r["scale"].size() >= 2) {
                element->Rect.Scale = {r["scale"][0].get<float>(), r["scale"][1].get<float>()};
            }
        }
    }

    static void DeserializeFlags(const json &j, UI::UIElement *element) {
        if (j.contains("flags")) {
            const uint8_t flags = j["flags"].get<uint8_t>();
            if (flags & 1)
                element->AddFlag(UI::VISIBLE);
            else
                element->RemoveFlag(UI::VISIBLE);
            if (flags & 2)
                element->AddFlag(UI::ENABLED);
            else
                element->RemoveFlag(UI::ENABLED);
        }
    }

    static void DeserializeColor(const json &j, const char *key, glm::vec4 &out) {
        if (j.contains(key) && j[key].is_array() && j[key].size() >= 4) {
            out = {j[key][0].get<float>(), j[key][1].get<float>(), j[key][2].get<float>(), j[key][3].get<float>()};
        }
    }

    static std::unique_ptr<UI::UIElement> DeserializeElement(const json &j, Core::ResourceManager &resources) {
        const std::string type = j.value("type", "Element");
        std::unique_ptr<UI::UIElement> element;

        if (type == "Text") {
            element = std::make_unique<UI::UIText>();
            auto *text = static_cast<UI::UIText *>(element.get());
            glm::vec4 color = text->GetColor();
            DeserializeColor(j, "color", color);
            text->SetColor(color);
            text->SetText(j.value("text", ""));
            if (j.contains("font")) {
                const std::string fontKey = j["font"].get<std::string>();
                auto font = resources.Get<UI::Font>(fontKey);
                if (font) {
                    text->SetFont(font);
                } else {
                    LOG_WARN(LOG_WHO, "Font '" + fontKey + "' not found in resources. Text will use default font.");
                }
            }
        } else if (type == "Button") {
            element = std::make_unique<UI::UIButton>();
            auto *btn = static_cast<UI::UIButton *>(element.get());
            glm::vec4 color = btn->GetColor();
            glm::vec4 bgColor = btn->GetBackgroundColor();
            glm::vec4 hoverBgColor = btn->GetHoveredBackgroundColor();
            DeserializeColor(j, "color", color);
            DeserializeColor(j, "bg_color", bgColor);
            DeserializeColor(j, "hovered_bg_color", hoverBgColor);
            btn->SetColor(color);
            btn->SetBackgroundColor(bgColor);
            btn->SetHoveredBackgroundColor(hoverBgColor);
            btn->SetText(j.value("text", ""));
            if (j.contains("bg_texture")) {
                const std::string texKey = j["bg_texture"].get<std::string>();
                auto tex = resources.Get<Rendering::Texture>(texKey);
                if (tex) {
                    btn->SetBackgroundTexture(tex);
                } else {
                    LOG_WARN(LOG_WHO, "Texture '" + texKey + "' not found in resources for button background.");
                }
            }
            if (j.contains("font")) {
                const std::string fontKey = j["font"].get<std::string>();
                auto font = resources.Get<UI::Font>(fontKey);
                if (font) {
                    btn->SetFont(font);
                } else {
                    LOG_WARN(LOG_WHO, "Font '" + fontKey + "' not found in resources. Button will use default font.");
                }
            }
        } else if (type == "Image") {
            element = std::make_unique<UI::UIImage>();
            auto *img = static_cast<UI::UIImage *>(element.get());
            glm::vec4 color = img->GetColor();
            DeserializeColor(j, "color", color);
            img->SetColor(color);
            if (j.contains("texture")) {
                const std::string texKey = j["texture"].get<std::string>();
                auto tex = resources.Get<Rendering::Texture>(texKey);
                if (tex) {
                    img->SetImage(tex);
                } else {
                    LOG_WARN(LOG_WHO, "Texture '" + texKey + "' not found in resources.");
                }
            }
        } else if (type == "Rect") {
            element = std::make_unique<UI::UIRect>();
            auto *rect = static_cast<UI::UIRect *>(element.get());
            glm::vec4 color = rect->GetColor();
            DeserializeColor(j, "color", color);
            rect->SetColor(color);
        } else {
            element = std::make_unique<UI::UIElement>();
        }

        element->Name = j.value("name", "Unnamed");
        DeserializeRect(j, element.get());
        DeserializeFlags(j, element.get());

        return element;
    }

    static void DeserializeChildren(const json &j, UI::UIElement *parent, UI::UISystem &uiSystem, Core::ResourceManager &resources) {
        if (!j.contains("children"))
            return;

        for (const auto &childJson : j["children"]) {
            std::unique_ptr<UI::UIElement> element;
            try {
                element = DeserializeElement(childJson, resources);
            } catch (const std::exception &e) {
                LOG_ERROR(LOG_WHO, std::string("Failed to deserialize UI child element: ") + e.what());
                continue;
            }
            if (!element)
                continue;

            UI::UIElement *raw = uiSystem.AddChild(parent, std::move(element));

            // recurse for nested children
            DeserializeChildren(childJson, raw, uiSystem, resources);
        }
    }

    void Serialize(json &out, UI::UISystem &uiSystem, Core::ResourceManager &resources) {
        const auto *root = uiSystem.GetRoot();
        if (!root)
            return;

        out["ui"]["elements"] = json::array();

        for (const auto *child : root->Children) {
            json elementJson;
            SerializeElement(elementJson, child, resources);
            out["ui"]["elements"].push_back(std::move(elementJson));
        }
    }

    bool Deserialize(const json &uiJson, UI::UISystem &uiSystem, Core::ResourceManager &resources) {
        if (!uiJson.contains("elements"))
            return true;

        auto *root = uiSystem.GetRoot();
        if (!root) {
            LOG_ERROR(LOG_WHO, "No root element in UISystem");
            return false;
        }

        uiSystem.Clear();

        for (const auto &elementJson : uiJson["elements"]) {
            std::unique_ptr<UI::UIElement> element;
            try {
                element = DeserializeElement(elementJson, resources);
            } catch (const std::exception &e) {
                LOG_ERROR(LOG_WHO, std::string("Failed to deserialize UI element: ") + e.what());
                continue;
            }
            if (!element)
                continue;

            UI::UIElement *raw = uiSystem.AddChild(root, std::move(element));
            DeserializeChildren(elementJson, raw, uiSystem, resources);
        }

        return true;
    }

} // namespace IO::UISerializer

#pragma pop_macro("LOG_WHO")
