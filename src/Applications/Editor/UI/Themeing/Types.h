#pragma once
#include "imgui.h"
#include "imgui_internal.h"
#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace Editor::UI::Theme {

    //
    // types
    //

    // theme
    struct Theme {
        std::string name = "UntitledTheme";

        std::array<std::optional<ImVec4>, ImGuiCol_COUNT> colors{};
        std::array<std::optional<float>, ImGuiStyleVar_COUNT> floatVars{};
        std::array<std::optional<ImVec2>, ImGuiStyleVar_COUNT> vec2Vars{};

        void SetColor(const ImGuiCol_ id, const ImVec4 &value) noexcept {
            if (const auto idx = static_cast<std::size_t>(id); idx < colors.size()) {
                colors[idx] = value;
            }
        }

        void SetFloat(const ImGuiStyleVar_ id, float value) noexcept {
            if (const auto idx = static_cast<std::size_t>(id); idx < floatVars.size()) {
                floatVars[idx] = value;
            }
        }

        void SetVec2(const ImGuiStyleVar_ id, const ImVec2 &value) noexcept {
            if (const auto idx = static_cast<std::size_t>(id); idx < vec2Vars.size()) {
                vec2Vars[idx] = value;
            }
        }

        [[nodiscard]] std::optional<ImVec4> GetColor(const ImGuiCol_ id) const noexcept {
            const auto idx = static_cast<std::size_t>(id);
            return idx < colors.size() ? colors[idx] : std::nullopt;
        }

        [[nodiscard]] std::optional<float> GetFloat(const ImGuiStyleVar_ id) const noexcept {
            const auto idx = static_cast<std::size_t>(id);
            return idx < floatVars.size() ? floatVars[idx] : std::nullopt;
        }

        [[nodiscard]] std::optional<ImVec2> GetVec2(const ImGuiStyleVar_ id) const noexcept {
            const auto idx = static_cast<std::size_t>(id);
            return idx < vec2Vars.size() ? vec2Vars[idx] : std::nullopt;
        }
    };

    // semantic palette

    struct SemanticPalette {
        ImVec4 bg{0.098f, 0.086f, 0.129f, 1.00f};
        ImVec4 bgAlt{0.141f, 0.122f, 0.180f, 1.00f};
        ImVec4 bgActive{0.192f, 0.161f, 0.243f, 1.00f};
        ImVec4 accent{0.545f, 0.361f, 0.902f, 1.00f};
        ImVec4 accentHover{0.639f, 0.463f, 0.980f, 1.00f};
        ImVec4 accentActive{0.451f, 0.278f, 0.784f, 1.00f};
        ImVec4 text{0.922f, 0.902f, 0.961f, 1.00f};
        ImVec4 textDim{0.545f, 0.514f, 0.596f, 1.00f};
        ImVec4 border{0.055f, 0.047f, 0.075f, 1.00f};
    };


    // Fonts

    enum class FontRole : uint8_t {
        Body,
        Bold,
        Monospace,
        Small,
        Heading,
        Icons,
    };

    struct FontConfig {
        std::string name;
        std::filesystem::path path;
        float sizePixels = 16.0f;
        FontRole role = FontRole::Body;
        bool mergeIntoPrevious = false;
        float iconMinAdvanceX = 0.0f;
        ImFont *fontPtr = nullptr;
    };

    struct FontSet {
        std::vector<FontConfig> fonts;


        [[nodiscard]] const FontConfig *Find(const FontRole role) const noexcept {
            const auto it = std::ranges::find(fonts, role, &FontConfig::role);
            return it != fonts.end() ? &*it : nullptr;
        }

        // wrapper to return ImFont pointer for use with ImGui::PushFont etc
        [[nodiscard]] ImFont *FindFont(const FontRole role) const noexcept {
            const auto f = Find(role);
            return f == nullptr ? ImGui::GetDefaultFont() : f->fontPtr;
        }
    };

    // style
    using FloatMember = float ImGuiStyle::*;
    using Vec2Member = ImVec2 ImGuiStyle::*;

    struct StyleVarInfo {
        ImGuiStyleVar_ id;
        std::string_view name;
        std::variant<FloatMember, Vec2Member> member;
    };


    // needed methods
    [[nodiscard]] constexpr ImVec4 Lerp(const ImVec4 &a, const ImVec4 &b, const float t) noexcept { return {std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t), std::lerp(a.z, b.z, t), std::lerp(a.w, b.w, t)}; }
    [[nodiscard]] inline Theme BuildFromPalette(const SemanticPalette &p, std::string name = "Custom") {
        Theme t;
        t.name = std::move(name);

        t.SetColor(ImGuiCol_WindowBg, p.bg);
        t.SetColor(ImGuiCol_ChildBg, p.bg);
        t.SetColor(ImGuiCol_PopupBg, p.bg);
        t.SetColor(ImGuiCol_MenuBarBg, p.bgAlt);
        t.SetColor(ImGuiCol_TitleBg, p.bg);
        t.SetColor(ImGuiCol_TitleBgActive, p.bgAlt);
        t.SetColor(ImGuiCol_TitleBgCollapsed, p.bg);
        t.SetColor(ImGuiCol_Border, p.border);

        t.SetColor(ImGuiCol_FrameBg, p.bgAlt);
        t.SetColor(ImGuiCol_FrameBgHovered, p.bgActive);
        t.SetColor(ImGuiCol_FrameBgActive, Lerp(p.bgActive, p.accent, 0.25f));

        t.SetColor(ImGuiCol_Button, p.accent);
        t.SetColor(ImGuiCol_ButtonHovered, p.accentHover);
        t.SetColor(ImGuiCol_ButtonActive, p.accentActive);

        t.SetColor(ImGuiCol_Header, Lerp(p.bgAlt, p.accent, 0.5f));
        t.SetColor(ImGuiCol_HeaderHovered, p.accentHover);
        t.SetColor(ImGuiCol_HeaderActive, p.accentActive);

        t.SetColor(ImGuiCol_TabHovered, p.accentHover);
        t.SetColor(ImGuiCol_Tab, p.bgAlt);
        t.SetColor(ImGuiCol_TabSelected, Lerp(p.bgActive, p.accent, 0.4f));
        t.SetColor(ImGuiCol_TabSelectedOverline, p.accent);
        t.SetColor(ImGuiCol_TabDimmed, p.bg);
        t.SetColor(ImGuiCol_TabDimmedSelected, p.bgAlt);
        t.SetColor(ImGuiCol_TabDimmedSelectedOverline, p.textDim);

        t.SetColor(ImGuiCol_CheckMark, p.accent);
        t.SetColor(ImGuiCol_SliderGrab, p.accent);
        t.SetColor(ImGuiCol_SliderGrabActive, p.accentActive);

        t.SetColor(ImGuiCol_ScrollbarBg, p.bg);
        t.SetColor(ImGuiCol_ScrollbarGrab, p.bgActive);
        t.SetColor(ImGuiCol_ScrollbarGrabHovered, Lerp(p.bgActive, p.accent, 0.3f));
        t.SetColor(ImGuiCol_ScrollbarGrabActive, p.accent);

        t.SetColor(ImGuiCol_Separator, p.border);
        t.SetColor(ImGuiCol_SeparatorHovered, p.accentHover);
        t.SetColor(ImGuiCol_SeparatorActive, p.accentActive);

        t.SetColor(ImGuiCol_ResizeGrip, p.bgActive);
        t.SetColor(ImGuiCol_ResizeGripHovered, p.accentHover);
        t.SetColor(ImGuiCol_ResizeGripActive, p.accentActive);

        t.SetColor(ImGuiCol_Text, p.text);
        t.SetColor(ImGuiCol_TextDisabled, p.textDim);
        t.SetColor(ImGuiCol_TextLink, p.accentHover);
        t.SetColor(ImGuiCol_TextSelectedBg, Lerp(p.bg, p.accent, 0.35f));

        t.SetColor(ImGuiCol_TableHeaderBg, p.bgAlt);
        t.SetColor(ImGuiCol_TableBorderStrong, p.border);
        t.SetColor(ImGuiCol_TableBorderLight, Lerp(p.border, p.bg, 0.5f));
        t.SetColor(ImGuiCol_TableRowBg, p.bg);
        t.SetColor(ImGuiCol_TableRowBgAlt, p.bgAlt);

        t.SetColor(ImGuiCol_DockingPreview, Lerp(p.bg, p.accent, 0.5f));
        t.SetColor(ImGuiCol_DockingEmptyBg, p.bg);

        t.SetColor(ImGuiCol_NavCursor, p.accent);
        t.SetColor(ImGuiCol_NavWindowingHighlight, p.text);
        t.SetColor(ImGuiCol_NavWindowingDimBg, ImVec4(p.bg.x, p.bg.y, p.bg.z, 0.6f));
        t.SetColor(ImGuiCol_ModalWindowDimBg, ImVec4(p.bg.x, p.bg.y, p.bg.z, 0.6f));

        t.SetFloat(ImGuiStyleVar_WindowRounding, 10.0f);
        t.SetFloat(ImGuiStyleVar_ChildRounding, 5.0f);
        t.SetFloat(ImGuiStyleVar_PopupRounding, 3.0f);
        t.SetFloat(ImGuiStyleVar_FrameRounding, 3.0f);
        t.SetFloat(ImGuiStyleVar_GrabRounding, 3.0f);
        t.SetFloat(ImGuiStyleVar_TabRounding, 3.0f);
        t.SetFloat(ImGuiStyleVar_ScrollbarRounding, 6.0f);
        t.SetFloat(ImGuiStyleVar_WindowBorderSize, 1.0f);
        t.SetFloat(ImGuiStyleVar_FrameBorderSize, 0.0f);
        t.SetFloat(ImGuiStyleVar_TabBorderSize, 0.0f);
        t.SetFloat(ImGuiStyleVar_TabBarBorderSize, 1.0f);
        t.SetFloat(ImGuiStyleVar_ScrollbarSize, 12.0f);
        t.SetFloat(ImGuiStyleVar_GrabMinSize, 8.0f);
        t.SetFloat(ImGuiStyleVar_DockingSeparatorSize, 2.0f);
        t.SetVec2(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
        t.SetVec2(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
        t.SetVec2(ImGuiStyleVar_ItemSpacing, ImVec2(6, 6));
        t.SetVec2(ImGuiStyleVar_CellPadding, ImVec2(6, 4));

        return t;
    }


    //
    // Tables
    //

    inline constexpr std::array kStyleVarTable{
            /*
            StyleVarInfo{ImGuiStyleVar_Alpha, "Alpha", &ImGuiStyle::Alpha},
            StyleVarInfo{ImGuiStyleVar_DisabledAlpha, "DisabledAlpha", &ImGuiStyle::DisabledAlpha},
            */
            StyleVarInfo{.id = ImGuiStyleVar_WindowPadding, .name = "WindowPadding", .member = &ImGuiStyle::WindowPadding},
            StyleVarInfo{.id = ImGuiStyleVar_WindowRounding, .name = "WindowRounding", .member = &ImGuiStyle::WindowRounding},
            StyleVarInfo{.id = ImGuiStyleVar_WindowBorderSize, .name = "WindowBorderSize", .member = &ImGuiStyle::WindowBorderSize},

            StyleVarInfo{.id = ImGuiStyleVar_ChildRounding, .name = "ChildRounding", .member = &ImGuiStyle::ChildRounding},
            StyleVarInfo{.id = ImGuiStyleVar_ChildBorderSize, .name = "ChildBorderSize", .member = &ImGuiStyle::ChildBorderSize},

            StyleVarInfo{.id = ImGuiStyleVar_PopupRounding, .name = "PopupRounding", .member = &ImGuiStyle::PopupRounding},
            StyleVarInfo{.id = ImGuiStyleVar_PopupBorderSize, .name = "PopupBorderSize", .member = &ImGuiStyle::PopupBorderSize},

            StyleVarInfo{.id = ImGuiStyleVar_FramePadding, .name = "FramePadding", .member = &ImGuiStyle::FramePadding},
            StyleVarInfo{.id = ImGuiStyleVar_FrameRounding, .name = "FrameRounding", .member = &ImGuiStyle::FrameRounding},
            StyleVarInfo{.id = ImGuiStyleVar_FrameBorderSize, .name = "FrameBorderSize", .member = &ImGuiStyle::FrameBorderSize},

            StyleVarInfo{.id = ImGuiStyleVar_ItemSpacing, .name = "ItemSpacing", .member = &ImGuiStyle::ItemSpacing},
            StyleVarInfo{.id = ImGuiStyleVar_ItemInnerSpacing, .name = "ItemInnerSpacing", .member = &ImGuiStyle::ItemInnerSpacing},
            StyleVarInfo{.id = ImGuiStyleVar_IndentSpacing, .name = "IndentSpacing", .member = &ImGuiStyle::IndentSpacing},
            StyleVarInfo{.id = ImGuiStyleVar_CellPadding, .name = "CellPadding", .member = &ImGuiStyle::CellPadding},

            StyleVarInfo{.id = ImGuiStyleVar_ScrollbarSize, .name = "ScrollbarSize", .member = &ImGuiStyle::ScrollbarSize},
            StyleVarInfo{.id = ImGuiStyleVar_ScrollbarRounding, .name = "ScrollbarRounding", .member = &ImGuiStyle::ScrollbarRounding},

            StyleVarInfo{.id = ImGuiStyleVar_GrabMinSize, .name = "GrabMinSize", .member = &ImGuiStyle::GrabMinSize},
            StyleVarInfo{.id = ImGuiStyleVar_GrabRounding, .name = "GrabRounding", .member = &ImGuiStyle::GrabRounding},

            StyleVarInfo{.id = ImGuiStyleVar_ImageBorderSize, .name = "ImageBorderSize", .member = &ImGuiStyle::ImageBorderSize},

            StyleVarInfo{.id = ImGuiStyleVar_TabRounding, .name = "TabRounding", .member = &ImGuiStyle::TabRounding},
            StyleVarInfo{.id = ImGuiStyleVar_TabBorderSize, .name = "TabBorderSize", .member = &ImGuiStyle::TabBorderSize},
            StyleVarInfo{.id = ImGuiStyleVar_TabBarBorderSize, .name = "TabBarBorderSize", .member = &ImGuiStyle::TabBarBorderSize},

            StyleVarInfo{.id = ImGuiStyleVar_SeparatorTextBorderSize, .name = "SeparatorTextBorderSize", .member = &ImGuiStyle::SeparatorTextBorderSize},
            StyleVarInfo{.id = ImGuiStyleVar_DockingSeparatorSize, .name = "DockingSeparatorSize", .member = &ImGuiStyle::DockingSeparatorSize},
    };


} // namespace Editor::UI::Theme
