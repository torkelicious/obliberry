#pragma once

#include "EditorTheme.h"
#include "Core/Utils/PathUtils.h"
#include "IO/VFS/VFS.h"

#include <fstream>
#include <nlohmann/json.hpp>

#pragma push_macro("LOG_WHO")
#define LOG_WHO "ThemeSerializer"

// Theme / FontSet serialization
// The file lives alongside the executable (similar to imgui.ini)

namespace Editor::UI::Theme::IO {

    using json = nlohmann::json;

    // Themes

    inline json ThemeToJson(const Theme &theme) {
        json j;

        j["name"] = theme.name;

        // Colors
        json colorsObj = json::object();

        for (const auto &[id, name] : kColorTable) {
            if (auto col = theme.GetColor(id)) {
                colorsObj[std::string(name)] = {col->x, col->y, col->z, col->w};
            }
        }

        j["colors"] = colorsObj;

        // Stylevars
        json varsObj = json::object();

        for (const auto &info : kStyleVarTable) {
            std::visit(
                    [&](auto memberPtr) {
                        using MemberT = std::decay_t<decltype(memberPtr)>;

                        if constexpr (std::is_same_v<MemberT, FloatMember>) {
                            if (auto val = theme.GetFloat(info.id)) {
                                varsObj[std::string(info.name)] = *val;
                            }
                        } else {
                            if (auto val = theme.GetVec2(info.id)) {
                                varsObj[std::string(info.name)] = {val->x, val->y};
                            }
                        }
                    },
                    info.member);
        }

        j["vars"] = varsObj;

        return j;
    }

    inline Theme ThemeFromJson(const json &j) {
        Theme theme;

        // Name
        if (j.contains("name") && j["name"].is_string()) {
            theme.name = j["name"].get<std::string>();
        }

        // Colors
        if (j.contains("colors") && j["colors"].is_object()) {
            for (const auto &[name, val] : j["colors"].items()) {
                if (auto id = ColorFromName(name); id && val.is_array() && val.size() == 4) {
                    theme.SetColor(*id, ImVec4(val[0].get<float>(), val[1].get<float>(), val[2].get<float>(), val[3].get<float>()));
                }
            }
        }

        // Stylevars
        if (j.contains("vars") && j["vars"].is_object()) {
            for (const auto &[name, val] : j["vars"].items()) {
                if (const auto *info = FindStyleVar(name)) {
                    std::visit(
                            [&](auto memberPtr) {
                                using MemberT = std::decay_t<decltype(memberPtr)>;

                                if constexpr (std::is_same_v<MemberT, FloatMember>) {
                                    if (val.is_number()) {
                                        theme.SetFloat(info->id, val.get<float>());
                                    }
                                } else {
                                    if (val.is_array() && val.size() == 2) {
                                        theme.SetVec2(info->id, ImVec2(val[0].get<float>(), val[1].get<float>()));
                                    }
                                }
                            },
                            info->member);
                }
            }
        }

        return theme;
    }

    // FontSets

    inline json FontSetToJson(const FontSet &set) {
        json j;
        j["fonts"] = json::array();

        for (const auto &font : set.fonts) {
            j["fonts"].push_back({{"name", font.name},
                                  {"path", font.path.string()},
                                  {"sizePixels", font.sizePixels},
                                  {"role", static_cast<int>(font.role)},
                                  {"mergeIntoPrevious", font.mergeIntoPrevious},
                                  {"iconMinAdvanceX", font.iconMinAdvanceX}});
        }

        return j;
    }

    inline FontSet JsonToFontSet(const json &j) {
        FontSet set;

        if (!j.contains("fonts") || !j["fonts"].is_array()) {
            return set;
        }

        for (const auto &fontJson : j["fonts"]) {
            if (!fontJson.is_object()) {
                continue;
            }

            FontConfig font;

            font.name = fontJson.value("name", "");
            font.path = fontJson.value("path", "");
            font.sizePixels = fontJson.value("sizePixels", 16.0f);

            const int role = fontJson.value("role", 0);
            font.role = static_cast<FontRole>(role);

            font.mergeIntoPrevious = fontJson.value("mergeIntoPrevious", false);

            font.iconMinAdvanceX = fontJson.value("iconMinAdvanceX", 0.0f);

            font.fontPtr = nullptr;

            set.fonts.push_back(std::move(font));
        }

        return set;
    }

    // file io

    inline bool Serialize(const EditorContext &ctx, const std::filesystem::path &path = Core::PathUtils::GetExecutableDirectory() / "theme.json") {
        std::ofstream file(path);

        if (!file.is_open()) {
            LOG_WARN(LOG_WHO, "Couldnt serialize theme");
            return false;
        }

        json j = ThemeToJson(ctx.theme);
        j["fontset"] = FontSetToJson(ctx.fontset);

        file << j.dump(4);

        if (!file.good()) {
            LOG_WARN(LOG_WHO, "Failed writing theme file");
            return false;
        }

        LOG_INFO(LOG_WHO, "Serialized theme");

        return true;
    }

    inline bool Deserialize(EditorContext &ctx, const std::filesystem::path &path = Core::PathUtils::GetExecutableDirectory() / "theme.json") {
        std::ifstream file(path);

        if (!file.is_open()) {
            LOG_WARN(LOG_WHO, "Couldnt deserialize theme");
            return false;
        }

        const auto j = json::parse(file, nullptr,
                                   /* allow_exceptions = */ false);

        if (j.is_discarded() || !j.is_object()) {
            LOG_WARN(LOG_WHO, "Invalid theme JSON");
            return false;
        }

        ctx.theme = ThemeFromJson(j);
        if (j.contains("fontset") && j["fontset"].is_object()) {
            ctx.fontset = JsonToFontSet(j["fontset"]);
        }
        LOG_INFO(LOG_WHO, "Loaded theme successfully");
        return true;
    }
} // namespace Editor::UI::Theme::IO

#pragma pop_macro("LOG_WHO")
