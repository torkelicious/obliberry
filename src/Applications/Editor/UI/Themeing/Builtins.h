#pragma once

#include "Core/Constants.h"
#include "Core/Utils/PathUtils.h"
#include "Types.h"
#include "imgui.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace Editor::UI::Theme {

    enum class BuiltInTheme : std::uint8_t {
        ObliberryDark,
        ImGuiClassicDark,
        ObliberryLight,
        HighContrastDark,
        TerminalGreen,
    };

    struct BuiltInThemeInfo {
        BuiltInTheme id;
        std::string_view name;
    };

    inline constexpr std::array kBuiltInThemes{
            BuiltInThemeInfo{BuiltInTheme::ObliberryDark, "Obliberry Dark"},   BuiltInThemeInfo{BuiltInTheme::ImGuiClassicDark, "ImGui Classic Dark"},
            BuiltInThemeInfo{BuiltInTheme::ObliberryLight, "Obliberry Light"}, BuiltInThemeInfo{BuiltInTheme::HighContrastDark, "High Contrast Dark"},
            BuiltInThemeInfo{BuiltInTheme::TerminalGreen, "Haxxorman Green"},
    };

    [[nodiscard]] inline ImVec4 ColorFromHex(const std::uint32_t rgb, const float alpha = 1.0f) noexcept {

        return {
                static_cast<float>((rgb >> 16U) & 0xffU) / 255.0f,
                static_cast<float>((rgb >> 8U) & 0xffU) / 255.0f,
                static_cast<float>(rgb & 0xffU) / 255.0f,
                alpha,
        };
    }

    [[nodiscard]] inline Theme ThemeFromImGuiStyle(const ImGuiStyle &style, std::string name) {

        Theme theme;
        theme.name = std::move(name);

        for (std::size_t i = 0; i < ImGuiCol_COUNT; ++i) {
            theme.colors[i] = style.Colors[i];
        }

        for (const auto &info : kStyleVarTable) {
            const auto index = static_cast<std::size_t>(info.id);

            if (index >= ImGuiStyleVar_COUNT) {
                continue;
            }

            std::visit(
                    [&](auto member) {
                        using MemberType = std::decay_t<decltype(member)>;

                        if constexpr (std::is_same_v<MemberType, FloatMember>) {
                            theme.floatVars[index] = style.*member;
                        } else {
                            theme.vec2Vars[index] = style.*member;
                        }
                    },
                    info.member);
        }

        return theme;
    }

    inline void MergeTheme(Theme &destination, const Theme &overrides) {
        for (std::size_t i = 0; i < destination.colors.size(); ++i) {
            if (overrides.colors[i]) {
                destination.colors[i] = overrides.colors[i];
            }
        }

        for (std::size_t i = 0; i < destination.floatVars.size(); ++i) {
            if (overrides.floatVars[i]) {
                destination.floatVars[i] = overrides.floatVars[i];
            }
        }

        for (std::size_t i = 0; i < destination.vec2Vars.size(); ++i) {
            if (overrides.vec2Vars[i]) {
                destination.vec2Vars[i] = overrides.vec2Vars[i];
            }
        }
    }

    [[nodiscard]] inline Theme BuildCompletePaletteTheme(const SemanticPalette &palette, std::string name, const bool lightBase = false) {

        ImGuiStyle baseStyle;

        if (lightBase) {
            ImGui::StyleColorsLight(&baseStyle);
        } else {
            ImGui::StyleColorsDark(&baseStyle);
        }

        Theme result = ThemeFromImGuiStyle(baseStyle, name);
        const Theme overrides = BuildFromPalette(palette, name);

        MergeTheme(result, overrides);
        result.name = std::move(name);

        return result;
    }

    [[nodiscard]] inline Theme DefaultDarkTheme() { return BuildCompletePaletteTheme(SemanticPalette{}, "Obliberry Dark"); }

    [[nodiscard]] inline Theme ImGuiClassicDarkTheme() {
        ImGuiStyle style;
        ImGui::StyleColorsDark(&style);

        style.WindowRounding = 0.0f;
        style.ChildRounding = 0.0f;
        style.PopupRounding = 0.0f;
        style.FrameRounding = 0.0f;
        style.ScrollbarRounding = 0.0f;
        style.GrabRounding = 0.0f;
        style.TabRounding = 0.0f;

        return ThemeFromImGuiStyle(style, "ImGui Classic Dark");
    }

    [[nodiscard]] inline Theme ObliberryLightTheme() {
        const SemanticPalette palette{
                .bg = ColorFromHex(0xF4F4F6),
                .bgAlt = ColorFromHex(0xE5E7EB),
                .bgActive = ColorFromHex(0xD1D5DB),
                .accent = ColorFromHex(0x6D4AFF),
                .accentHover = ColorFromHex(0x7C5CFF),
                .accentActive = ColorFromHex(0x5638D4),
                .text = ColorFromHex(0x1B1B1F),
                .textDim = ColorFromHex(0x65656F),
                .border = ColorFromHex(0xC5C7CE),
        };

        return BuildCompletePaletteTheme(palette, "Obliberry Light", true);
    }

    [[nodiscard]] inline Theme HighContrastDarkTheme() {
        const SemanticPalette palette{
                .bg = ColorFromHex(0x090A0C),
                .bgAlt = ColorFromHex(0x15171B),
                .bgActive = ColorFromHex(0x242830),
                .accent = ColorFromHex(0x2F81F7),
                .accentHover = ColorFromHex(0x58A6FF),
                .accentActive = ColorFromHex(0x1F6FEB),
                .text = ColorFromHex(0xFFFFFF),
                .textDim = ColorFromHex(0xB8BDC7),
                .border = ColorFromHex(0x58606B),
        };

        return BuildCompletePaletteTheme(palette, "High Contrast Dark");
    }

    [[nodiscard]] inline Theme TerminalGreenTheme() {
        const SemanticPalette palette{
                .bg = ColorFromHex(0x07100A),
                .bgAlt = ColorFromHex(0x0D1B11),
                .bgActive = ColorFromHex(0x17321F),
                .accent = ColorFromHex(0x33D17A),
                .accentHover = ColorFromHex(0x57E389),
                .accentActive = ColorFromHex(0x26A269),
                .text = ColorFromHex(0xD7FBE3),
                .textDim = ColorFromHex(0x78A889),
                .border = ColorFromHex(0x245533),
        };

        return BuildCompletePaletteTheme(palette, "Haxxorman Green");
    }

    [[nodiscard]] inline Theme CreateBuiltInTheme(const BuiltInTheme preset) {

        switch (preset) {
            case BuiltInTheme::ObliberryDark:
                return DefaultDarkTheme();

            case BuiltInTheme::ImGuiClassicDark:
                return ImGuiClassicDarkTheme();

            case BuiltInTheme::ObliberryLight:
                return ObliberryLightTheme();

            case BuiltInTheme::HighContrastDark:
                return HighContrastDarkTheme();

            case BuiltInTheme::TerminalGreen:
                return TerminalGreenTheme();
        }

        return DefaultDarkTheme();
    }

    [[nodiscard]] inline FontSet DefaultFontSet() {
        return FontSet{.fonts = {
                               {
                                       .name = "Inter Variable",
                                       .path = Core::PathUtils::Join(Core::E_EDITOR_FONTS_PATH, "Inter/Inter-VariableFont_opsz,wght.ttf"),
                                       .sizePixels = 16.0f,
                                       .role = FontRole::Body,
                               },
                               {
                                       .name = "Inter Bold",
                                       .path = Core::PathUtils::Join(Core::E_EDITOR_FONTS_PATH, "Inter/static/Inter_24pt-Bold.ttf"),
                                       .sizePixels = 24.0f,
                                       .role = FontRole::Bold,
                               },
                               {
                                       .name = "JetBrains Mono",
                                       .path = Core::PathUtils::Join(Core::E_EDITOR_FONTS_PATH, "JetBrains_Mono/JetBrainsMono-VariableFont_wght.ttf"),
                                       .sizePixels = 15.0f,
                                       .role = FontRole::Monospace,
                               },
                       }};
    }

} // namespace Editor::UI::Theme
