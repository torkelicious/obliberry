// editor theme data
// for ImGui 1.92.9b (docking)
//
#pragma once

#include "imgui.h"
#include "Core/Constants.h"
#include "Core/Utils/PathUtils.h"
#include "Platform/FreeType.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <format>
#include <imgui_internal.h>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>
#include <freetype/freetype.h>

namespace Editor::UI::Theme {

    // color table

    struct ColorInfo {
        ImGuiCol_ id;
        std::string_view name;
    };

    inline constexpr std::size_t kColorTableSize = 55;
    // lookups are by id not position
    inline constexpr std::array kColorTable{
            ColorInfo{.id = ImGuiCol_Text, .name = "Text"},
            ColorInfo{.id = ImGuiCol_TextDisabled, .name = "TextDisabled"},
            ColorInfo{.id = ImGuiCol_WindowBg, .name = "WindowBg"},
            ColorInfo{.id = ImGuiCol_ChildBg, .name = "ChildBg"},
            ColorInfo{.id = ImGuiCol_PopupBg, .name = "PopupBg"},
            ColorInfo{.id = ImGuiCol_Border, .name = "Border"},
            ColorInfo{.id = ImGuiCol_BorderShadow, .name = "BorderShadow"},
            ColorInfo{.id = ImGuiCol_FrameBg, .name = "FrameBg"},
            ColorInfo{.id = ImGuiCol_FrameBgHovered, .name = "FrameBgHovered"},
            ColorInfo{.id = ImGuiCol_FrameBgActive, .name = "FrameBgActive"},
            ColorInfo{.id = ImGuiCol_TitleBg, .name = "TitleBg"},
            ColorInfo{.id = ImGuiCol_TitleBgActive, .name = "TitleBgActive"},
            ColorInfo{.id = ImGuiCol_TitleBgCollapsed, .name = "TitleBgCollapsed"},
            ColorInfo{.id = ImGuiCol_MenuBarBg, .name = "MenuBarBg"},
            ColorInfo{.id = ImGuiCol_ScrollbarBg, .name = "ScrollbarBg"},
            ColorInfo{.id = ImGuiCol_ScrollbarGrab, .name = "ScrollbarGrab"},
            ColorInfo{.id = ImGuiCol_ScrollbarGrabHovered, .name = "ScrollbarGrabHovered"},
            ColorInfo{.id = ImGuiCol_ScrollbarGrabActive, .name = "ScrollbarGrabActive"},
            ColorInfo{.id = ImGuiCol_CheckMark, .name = "CheckMark"},
            ColorInfo{.id = ImGuiCol_SliderGrab, .name = "SliderGrab"},
            ColorInfo{.id = ImGuiCol_SliderGrabActive, .name = "SliderGrabActive"},
            ColorInfo{.id = ImGuiCol_Button, .name = "Button"},
            ColorInfo{.id = ImGuiCol_ButtonHovered, .name = "ButtonHovered"},
            ColorInfo{.id = ImGuiCol_ButtonActive, .name = "ButtonActive"},
            ColorInfo{.id = ImGuiCol_Header, .name = "Header"},
            ColorInfo{.id = ImGuiCol_HeaderHovered, .name = "HeaderHovered"},
            ColorInfo{.id = ImGuiCol_HeaderActive, .name = "HeaderActive"},
            ColorInfo{.id = ImGuiCol_Separator, .name = "Separator"},
            ColorInfo{.id = ImGuiCol_SeparatorHovered, .name = "SeparatorHovered"},
            ColorInfo{.id = ImGuiCol_SeparatorActive, .name = "SeparatorActive"},
            ColorInfo{.id = ImGuiCol_ResizeGrip, .name = "ResizeGrip"},
            ColorInfo{.id = ImGuiCol_ResizeGripHovered, .name = "ResizeGripHovered"},
            ColorInfo{.id = ImGuiCol_ResizeGripActive, .name = "ResizeGripActive"},
            ColorInfo{.id = ImGuiCol_TabHovered, .name = "TabHovered"},
            ColorInfo{.id = ImGuiCol_Tab, .name = "Tab"},
            ColorInfo{.id = ImGuiCol_TabSelected, .name = "TabSelected"},
            ColorInfo{.id = ImGuiCol_TabSelectedOverline, .name = "TabSelectedOverline"},
            ColorInfo{.id = ImGuiCol_TabDimmed, .name = "TabDimmed"},
            ColorInfo{.id = ImGuiCol_TabDimmedSelected, .name = "TabDimmedSelected"},
            ColorInfo{.id = ImGuiCol_TabDimmedSelectedOverline, .name = "TabDimmedSelectedOverline"},
            ColorInfo{.id = ImGuiCol_DockingPreview, .name = "DockingPreview"},
            ColorInfo{.id = ImGuiCol_DockingEmptyBg, .name = "DockingEmptyBg"},
            ColorInfo{.id = ImGuiCol_PlotLines, .name = "PlotLines"},
            ColorInfo{.id = ImGuiCol_PlotLinesHovered, .name = "PlotLinesHovered"},
            ColorInfo{.id = ImGuiCol_PlotHistogram, .name = "PlotHistogram"},
            ColorInfo{.id = ImGuiCol_PlotHistogramHovered, .name = "PlotHistogramHovered"},
            ColorInfo{.id = ImGuiCol_TableHeaderBg, .name = "TableHeaderBg"},
            ColorInfo{.id = ImGuiCol_TableBorderStrong, .name = "TableBorderStrong"},
            ColorInfo{.id = ImGuiCol_TableBorderLight, .name = "TableBorderLight"},
            ColorInfo{.id = ImGuiCol_TableRowBg, .name = "TableRowBg"},
            ColorInfo{.id = ImGuiCol_TableRowBgAlt, .name = "TableRowBgAlt"},
            ColorInfo{.id = ImGuiCol_TextLink, .name = "TextLink"},
            ColorInfo{.id = ImGuiCol_TextSelectedBg, .name = "TextSelectedBg"},
            ColorInfo{.id = ImGuiCol_DragDropTarget, .name = "DragDropTarget"},
            ColorInfo{.id = ImGuiCol_NavCursor, .name = "NavCursor"},
            ColorInfo{.id = ImGuiCol_NavWindowingHighlight, .name = "NavWindowingHighlight"},
            ColorInfo{.id = ImGuiCol_NavWindowingDimBg, .name = "NavWindowingDimBg"},
            ColorInfo{.id = ImGuiCol_ModalWindowDimBg, .name = "ModalWindowDimBg"},
    };


    [[nodiscard]] constexpr std::string_view ColorName(const ImGuiCol_ id) noexcept {
        const auto it = std::ranges::find(kColorTable, id, &ColorInfo::id);
        return it != kColorTable.end() ? it->name : "Unknown";
    }

    [[nodiscard]] constexpr std::optional<ImGuiCol_> ColorFromName(const std::string_view name) noexcept {
        const auto it = std::ranges::find(kColorTable, name, &ColorInfo::name);
        if (it == kColorTable.end())
            return std::nullopt;
        return it->id;
    }

    // style var table

    using FloatMember = float ImGuiStyle::*;
    using Vec2Member = ImVec2 ImGuiStyle::*;

    struct StyleVarInfo {
        ImGuiStyleVar_ id;
        std::string_view name;
        std::variant<FloatMember, Vec2Member> member;
    };

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

    inline void Apply(const Theme &theme) {
        ImGuiStyle &style = ImGui::GetStyle();

        for (std::size_t i = 0; i < ImGuiCol_COUNT; ++i) {
            if (theme.colors[i]) {
                style.Colors[i] = *theme.colors[i];
            }
        }

        for (const auto &info : kStyleVarTable) {
            const auto varIdx = static_cast<std::size_t>(info.id);
            if (varIdx >= ImGuiStyleVar_COUNT)
                continue;

            std::visit(
                    [&](auto memberPtr) {
                        using MemberT = std::decay_t<decltype(memberPtr)>;
                        if constexpr (std::is_same_v<MemberT, FloatMember>) {
                            if (const auto &v = theme.floatVars[varIdx]; v) {
                                style.*memberPtr = *v;
                            }
                        } else {
                            if (const auto &v = theme.vec2Vars[varIdx]; v) {
                                style.*memberPtr = *v;
                            }
                        }
                    },
                    info.member);
        }
    }

    inline void ApplyWithDpiScale(const Theme &theme, const float dpiScale) {
        Apply(theme);
        ImGui::GetStyle().ScaleAllSizes(dpiScale);
    }

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

    [[nodiscard]] inline Theme DefaultDarkTheme() { return BuildFromPalette(SemanticPalette{}, "DefaultDark"); }

    // Fonts

    // todo: use
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

    [[nodiscard]] inline FontSet DefaultFontSet() {
        // i love inter but idk wat im doing
        // todo:
        //  serialize fonts to theme.json
        //  add google fonts credit for inter / jetbrainsmono (both ofl)

        return FontSet{.fonts = {
                               {.name = "Inter Variable",


                                .path = Core::PathUtils::Join(Core::E_EDITOR_FONTS_PATH, "Inter/Inter-VariableFont_opsz,wght.ttf"),
                                .sizePixels = 16.0f,
                                .role = FontRole::Body},


                               {.name = "Inter Bold", .path = Core::PathUtils::Join(Core::E_EDITOR_FONTS_PATH, "Inter/static/Inter_24pt-Bold.ttf"), .sizePixels = 24.0f, .role = FontRole::Bold},

                               {.name = "JetBrains Mono", .path = Core::PathUtils::Join(Core::E_EDITOR_FONTS_PATH, "JetBrains_Mono/JetBrainsMono-VariableFont_wght.ttf"), .sizePixels = 15.0f, .role = FontRole::Monospace},
                       }};
    }

    inline std::string GetFontName(const std::filesystem::path &path) {
        FT_Face face = nullptr;
        if (FT_New_Face(FreeType::library(), path.string().c_str(), 0, &face) != 0) {
            return {};
        }
        std::string name = face->family_name ? face->family_name : "";
        FT_Done_Face(face);
        return name;
    }

    inline std::string GetFontStyle(const std::filesystem::path &path) {
        FT_Face face = nullptr;

        if (FT_New_Face(FreeType::library(), path.string().c_str(), 0, &face) != 0)
            return {};

        std::string style = face->style_name ? face->style_name : "";

        FT_Done_Face(face);

        return style;
    }

    inline std::string GetFullFontName(const std::filesystem::path &path) {
        FT_Face face = nullptr;

        if (FT_New_Face(FreeType::library(), path.string().c_str(), 0, &face) != 0)
            return {};

        std::string name = face->family_name ? face->family_name : "";

        if (face->style_name && *face->style_name) {
            if (!name.empty())
                name += ' ';
            name += face->style_name;
        }

        FT_Done_Face(face);
        return name;
    }

    // load a fontconfig directly from path
    inline FontConfig LoadFontConfig(const std::filesystem::path &path, const FontRole &role = FontRole::Body) {
        return {
                .name = GetFullFontName(path), .path = path, .sizePixels = 16.0f, .role = role
                /*explicitly avoiding assigning the ptr here, should only be assigned on apply*/
        };
    }

    inline void ApplyFontSet(FontSet &set) {
        // must be called outside the NewFrame() .. Render() scope
        ImGuiIO &io = ImGui::GetIO();

        LOG_INFO("Theme", "ApplyFontSet: Clearing font atlas, fonts in set: " + std::to_string(set.fonts.size()));
        io.Fonts->Clear();

        // role changes take visual effect.
        // Track the last non-merged font for mergeIntoPrevious to work correctly
        const ImFont *lastNonMergedFont = nullptr;
        for (constexpr FontRole roleOrder[] = {FontRole::Body, FontRole::Bold, FontRole::Monospace, FontRole::Small, FontRole::Heading, FontRole::Icons}; const FontRole role : roleOrder) {
            for (auto &font : set.fonts) {
                if (font.role == role) {
                    font.fontPtr = nullptr;
                    ImFontConfig cfg;
                    cfg.SizePixels = font.sizePixels;
                    cfg.MergeMode = font.mergeIntoPrevious;
                    cfg.GlyphMinAdvanceX = font.iconMinAdvanceX;
                    cfg.FontDataOwnedByAtlas = true;
                    LOG_INFO("Theme",
                             "Adding font: '" + font.name + "', role: " + std::to_string(static_cast<int>(role)) + ", size: " + std::to_string(font.sizePixels) + ", merge: " + std::to_string(font.mergeIntoPrevious));
                    font.fontPtr = io.Fonts->AddFontFromFileTTF(font.path.string().c_str(), font.sizePixels, &cfg);
                    if (!font.mergeIntoPrevious && font.fontPtr) {
                        lastNonMergedFont = font.fontPtr;
                    }
                }
            }
        }
        LOG_INFO("Theme", "Building font atlas...");
        io.Fonts->Build();
        LOG_INFO("Theme", "Font atlas built. Total fonts: " + std::to_string(io.Fonts->Fonts.Size));
        io.FontDefault = nullptr;
    }


    // serialization
    using KeyValueList = std::vector<std::pair<std::string, std::string>>;

    [[nodiscard]] inline std::string FormatVec4(const ImVec4 &c) { return std::format("{:.4f},{:.4f},{:.4f},{:.4f}", c.x, c.y, c.z, c.w); }

    [[nodiscard]] inline std::string FormatVec2(const ImVec2 &v) { return std::format("{:.4f},{:.4f}", v.x, v.y); }

    [[nodiscard]] inline std::string FormatFloat(float f) { return std::format("{:.4f}", f); }

    [[nodiscard]] inline bool ParseFloats(std::string_view s, std::span<float> out) noexcept {
        auto skipWs = [](std::string_view &sv) {
            while (!sv.empty() && (sv.front() == ' ' || sv.front() == '\t' || sv.front() == '\r' || sv.front() == '\n')) {
                sv.remove_prefix(1);
            }
        };

        for (float &val : out) {
            skipWs(s);
            if (s.empty())
                return false;

            const auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
            if (ec != std::errc{})
                return false;

            s.remove_prefix(static_cast<std::size_t>(ptr - s.data()));
            skipWs(s);

            if (!s.empty() && s.front() == ',') {
                s.remove_prefix(1);
            }
        }
        return true;
    }

    [[nodiscard]] inline KeyValueList SerializeToKV(const Theme &theme) {
        KeyValueList out;
        out.reserve(1 + kColorTable.size() + kStyleVarTable.size());

        out.emplace_back("__name", theme.name);

        for (const auto &[id, name] : kColorTable) {
            if (auto col = theme.GetColor(id)) {
                out.emplace_back(std::format("color.{}", name), FormatVec4(*col));
            }
        }

        for (const auto &info : kStyleVarTable) {
            std::visit(
                    [&](auto memberPtr) {
                        using MemberT = std::decay_t<decltype(memberPtr)>;
                        if constexpr (std::is_same_v<MemberT, FloatMember>) {
                            if (const auto val = theme.GetFloat(info.id)) {
                                out.emplace_back(std::format("var.{}", info.name), FormatFloat(*val));
                            }
                        } else {
                            if (const auto val = theme.GetVec2(info.id)) {
                                out.emplace_back(std::format("var.{}", info.name), FormatVec2(*val));
                            }
                        }
                    },
                    info.member);
        }
        return out;
    }

    [[nodiscard]] constexpr const StyleVarInfo *FindStyleVar(const std::string_view name) noexcept {
        const auto it = std::ranges::find(kStyleVarTable, name, &StyleVarInfo::name);
        return it != kStyleVarTable.end() ? &*it : nullptr;
    }

    [[nodiscard]] inline Theme DeserializeKV(const KeyValueList &kv) {
        Theme theme;

        for (const auto &[key, value] : kv) {
            if (key == "__name") {
                theme.name = value;
                continue;
            }

            if (key.starts_with("color.")) {
                if (auto id = ColorFromName(key.substr(6))) {
                    float f[4]{};
                    if (ParseFloats(value, f)) {
                        theme.SetColor(*id, ImVec4(f[0], f[1], f[2], f[3]));
                    }
                }
            } else if (key.starts_with("var.")) {
                if (const auto *info = FindStyleVar(key.substr(4))) {
                    std::visit(
                            [&](auto memberPtr) {
                                using MemberT = std::decay_t<decltype(memberPtr)>;
                                if constexpr (std::is_same_v<MemberT, FloatMember>) {
                                    float f = 0.0f;
                                    if (ParseFloats(value, std::span(&f, 1))) {
                                        theme.SetFloat(info->id, f);
                                    }
                                } else {
                                    float f[2]{};
                                    if (ParseFloats(value, f)) {
                                        theme.SetVec2(info->id, ImVec2(f[0], f[1]));
                                    }
                                }
                            },
                            info->member);
                }
            }
        }
        return theme;
    }
} // namespace Editor::UI::Theme
