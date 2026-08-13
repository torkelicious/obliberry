#pragma once
#include "EditorTheme.h"
#include "Core/Utils/PathUtils.h"
#include "IO/VFS/VFS.h"
#include <fstream>
#include <nlohmann/json.hpp>

#pragma push_macro("LOG_WHO")
#define LOG_WHO "ThemeSerializer"

// theme to json file, lives alongside executable (i.e think imgui.ini)
namespace Editor::UI::Theme::IO {
    using json = nlohmann::json;

    inline json ThemeToJson(const Theme &theme) {
        json j;
        j["name"] = theme.name;
        json colorsObj = json::object();
        for (const auto &[id, name] : kColorTable) {
            if (auto col = theme.GetColor(id)) {
                colorsObj[std::string(name)] = {col->x, col->y, col->z, col->w};
            }
        }
        j["colors"] = colorsObj;
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

        if (j.contains("name") && j["name"].is_string()) {
            theme.name = j["name"].get<std::string>();
        }
        if (j.contains("colors") && j["colors"].is_object()) {
            for (const auto &[name, val] : j["colors"].items()) {
                if (auto id = ColorFromName(name); id && val.is_array() && val.size() == 4) {
                    theme.SetColor(*id, ImVec4(val[0].get<float>(), val[1].get<float>(), val[2].get<float>(), val[3].get<float>()));
                }
            }
        }
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

    // file io

    inline bool Serialize(const Theme &theme, const std::filesystem::path &path = Core::PathUtils::GetExecutableDirectory() / "theme.json") {
        std::ofstream file(path);
        if (!file.is_open()) {
            return false;
        }

        file << ThemeToJson(theme).dump(4);

        LOG_INFO(LOG_WHO, "Serialized theme");
        return file.good();
    }

    inline bool Deserialize(Theme &theme, const std::filesystem::path &path = Core::PathUtils::GetExecutableDirectory() / "theme.json") {
        std::ifstream file(path);
        if (!file.is_open()) {
            LOG_WARN(LOG_WHO, "Couldnt deserialize theme");
            return false;
        }
        const auto j = json::parse(file, nullptr, /*allow_exceptions=*/false);
        if (j.is_discarded()) {
            return false;
        }
        theme = ThemeFromJson(j);
        LOG_INFO(LOG_WHO, "Loaded theme sucessfully");
        return true;
    }


} // namespace Editor::UI::Theme::IO

#pragma pop_macro("LOG_WHO")
