// editor theme data
// for ImGui 1.92.9 (docking)
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
            ColorInfo{ImGuiCol_Text, "Text"},
            ColorInfo{ImGuiCol_TextDisabled, "TextDisabled"},
            ColorInfo{ImGuiCol_WindowBg, "WindowBg"},
            ColorInfo{ImGuiCol_ChildBg, "ChildBg"},
            ColorInfo{ImGuiCol_PopupBg, "PopupBg"},
            ColorInfo{ImGuiCol_Border, "Border"},
            ColorInfo{ImGuiCol_BorderShadow, "BorderShadow"},
            ColorInfo{ImGuiCol_FrameBg, "FrameBg"},
            ColorInfo{ImGuiCol_FrameBgHovered, "FrameBgHovered"},
            ColorInfo{ImGuiCol_FrameBgActive, "FrameBgActive"},
            ColorInfo{ImGuiCol_TitleBg, "TitleBg"},
            ColorInfo{ImGuiCol_TitleBgActive, "TitleBgActive"},
            ColorInfo{ImGuiCol_TitleBgCollapsed, "TitleBgCollapsed"},
            ColorInfo{ImGuiCol_MenuBarBg, "MenuBarBg"},
            ColorInfo{ImGuiCol_ScrollbarBg, "ScrollbarBg"},
            ColorInfo{ImGuiCol_ScrollbarGrab, "ScrollbarGrab"},
            ColorInfo{ImGuiCol_ScrollbarGrabHovered, "ScrollbarGrabHovered"},
            ColorInfo{ImGuiCol_ScrollbarGrabActive, "ScrollbarGrabActive"},
            ColorInfo{ImGuiCol_CheckMark, "CheckMark"},
            ColorInfo{ImGuiCol_SliderGrab, "SliderGrab"},
            ColorInfo{ImGuiCol_SliderGrabActive, "SliderGrabActive"},
            ColorInfo{ImGuiCol_Button, "Button"},
            ColorInfo{ImGuiCol_ButtonHovered, "ButtonHovered"},
            ColorInfo{ImGuiCol_ButtonActive, "ButtonActive"},
            ColorInfo{ImGuiCol_Header, "Header"},
            ColorInfo{ImGuiCol_HeaderHovered, "HeaderHovered"},
            ColorInfo{ImGuiCol_HeaderActive, "HeaderActive"},
            ColorInfo{ImGuiCol_Separator, "Separator"},
            ColorInfo{ImGuiCol_SeparatorHovered, "SeparatorHovered"},
            ColorInfo{ImGuiCol_SeparatorActive, "SeparatorActive"},
            ColorInfo{ImGuiCol_ResizeGrip, "ResizeGrip"},
            ColorInfo{ImGuiCol_ResizeGripHovered, "ResizeGripHovered"},
            ColorInfo{ImGuiCol_ResizeGripActive, "ResizeGripActive"},
            ColorInfo{ImGuiCol_TabHovered, "TabHovered"},
            ColorInfo{ImGuiCol_Tab, "Tab"},
            ColorInfo{ImGuiCol_TabSelected, "TabSelected"},
            ColorInfo{ImGuiCol_TabSelectedOverline, "TabSelectedOverline"},
            ColorInfo{ImGuiCol_TabDimmed, "TabDimmed"},
            ColorInfo{ImGuiCol_TabDimmedSelected, "TabDimmedSelected"},
            ColorInfo{ImGuiCol_TabDimmedSelectedOverline, "TabDimmedSelectedOverline"},
            ColorInfo{ImGuiCol_DockingPreview, "DockingPreview"},
            ColorInfo{ImGuiCol_DockingEmptyBg, "DockingEmptyBg"},
            ColorInfo{ImGuiCol_PlotLines, "PlotLines"},
            ColorInfo{ImGuiCol_PlotLinesHovered, "PlotLinesHovered"},
            ColorInfo{ImGuiCol_PlotHistogram, "PlotHistogram"},
            ColorInfo{ImGuiCol_PlotHistogramHovered, "PlotHistogramHovered"},
            ColorInfo{ImGuiCol_TableHeaderBg, "TableHeaderBg"},
            ColorInfo{ImGuiCol_TableBorderStrong, "TableBorderStrong"},
            ColorInfo{ImGuiCol_TableBorderLight, "TableBorderLight"},
            ColorInfo{ImGuiCol_TableRowBg, "TableRowBg"},
            ColorInfo{ImGuiCol_TableRowBgAlt, "TableRowBgAlt"},
            ColorInfo{ImGuiCol_TextLink, "TextLink"},
            ColorInfo{ImGuiCol_TextSelectedBg, "TextSelectedBg"},
            ColorInfo{ImGuiCol_DragDropTarget, "DragDropTarget"},
            ColorInfo{ImGuiCol_NavCursor, "NavCursor"},
            ColorInfo{ImGuiCol_NavWindowingHighlight, "NavWindowingHighlight"},
            ColorInfo{ImGuiCol_NavWindowingDimBg, "NavWindowingDimBg"},
            ColorInfo{ImGuiCol_ModalWindowDimBg, "ModalWindowDimBg"},
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
            StyleVarInfo{ImGuiStyleVar_WindowPadding, "WindowPadding", &ImGuiStyle::WindowPadding},
            StyleVarInfo{ImGuiStyleVar_WindowRounding, "WindowRounding", &ImGuiStyle::WindowRounding},
            StyleVarInfo{ImGuiStyleVar_WindowBorderSize, "WindowBorderSize", &ImGuiStyle::WindowBorderSize},

            StyleVarInfo{ImGuiStyleVar_ChildRounding, "ChildRounding", &ImGuiStyle::ChildRounding},
            StyleVarInfo{ImGuiStyleVar_ChildBorderSize, "ChildBorderSize", &ImGuiStyle::ChildBorderSize},

            StyleVarInfo{ImGuiStyleVar_PopupRounding, "PopupRounding", &ImGuiStyle::PopupRounding},
            StyleVarInfo{ImGuiStyleVar_PopupBorderSize, "PopupBorderSize", &ImGuiStyle::PopupBorderSize},

            StyleVarInfo{ImGuiStyleVar_FramePadding, "FramePadding", &ImGuiStyle::FramePadding},
            StyleVarInfo{ImGuiStyleVar_FrameRounding, "FrameRounding", &ImGuiStyle::FrameRounding},
            StyleVarInfo{ImGuiStyleVar_FrameBorderSize, "FrameBorderSize", &ImGuiStyle::FrameBorderSize},

            StyleVarInfo{ImGuiStyleVar_ItemSpacing, "ItemSpacing", &ImGuiStyle::ItemSpacing},
            StyleVarInfo{ImGuiStyleVar_ItemInnerSpacing, "ItemInnerSpacing", &ImGuiStyle::ItemInnerSpacing},
            StyleVarInfo{ImGuiStyleVar_IndentSpacing, "IndentSpacing", &ImGuiStyle::IndentSpacing},
            StyleVarInfo{ImGuiStyleVar_CellPadding, "CellPadding", &ImGuiStyle::CellPadding},

            StyleVarInfo{ImGuiStyleVar_ScrollbarSize, "ScrollbarSize", &ImGuiStyle::ScrollbarSize},
            StyleVarInfo{ImGuiStyleVar_ScrollbarRounding, "ScrollbarRounding", &ImGuiStyle::ScrollbarRounding},

            StyleVarInfo{ImGuiStyleVar_GrabMinSize, "GrabMinSize", &ImGuiStyle::GrabMinSize},
            StyleVarInfo{ImGuiStyleVar_GrabRounding, "GrabRounding", &ImGuiStyle::GrabRounding},

            StyleVarInfo{ImGuiStyleVar_ImageBorderSize, "ImageBorderSize", &ImGuiStyle::ImageBorderSize},

            StyleVarInfo{ImGuiStyleVar_TabRounding, "TabRounding", &ImGuiStyle::TabRounding},
            StyleVarInfo{ImGuiStyleVar_TabBorderSize, "TabBorderSize", &ImGuiStyle::TabBorderSize},
            StyleVarInfo{ImGuiStyleVar_TabBarBorderSize, "TabBarBorderSize", &ImGuiStyle::TabBarBorderSize},

            StyleVarInfo{ImGuiStyleVar_SeparatorTextBorderSize, "SeparatorTextBorderSize", &ImGuiStyle::SeparatorTextBorderSize},
            StyleVarInfo{ImGuiStyleVar_DockingSeparatorSize, "DockingSeparatorSize", &ImGuiStyle::DockingSeparatorSize},
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
        ImFont *lastNonMergedFont = nullptr;
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
