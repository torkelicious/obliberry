// editor theme data
// for ImGui 1.92.9b (docking)
//
#pragma once

#include "imgui.h"
#include "Platform/FreeType.h"
#include <algorithm>
#include <array>
#include <format>
#include <imgui_internal.h>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <freetype/freetype.h>
#include "Types.h"

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

            std::visit([&](auto memberPtr) {
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
            }, info.member);
        }
    }

    inline void ApplyWithDpiScale(const Theme &theme, const float dpiScale) {
        Apply(theme);
        ImGui::GetStyle().ScaleAllSizes(dpiScale);
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
            std::visit([&](auto memberPtr) {
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
            }, info.member);
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
                    std::visit([&](auto memberPtr) {
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
                    }, info->member);
                }
            }
        }
        return theme;
    }
} // namespace Editor::UI::Theme
