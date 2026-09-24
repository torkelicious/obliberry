#include "AnimationSerialization.h"
#include "Core/ResourceManager.h"
#include "ECS/Systems/Animation/Animation.h"
#include "ECS/Systems/Animation/Types.h"
#include "IO/VFS/VFS.h"
#include "Logger/LoggerService.h"
#include "Rendering/Types/Texture/SpriteSheet.h"
#include "Rendering/Types/Texture/Texture.h"
#include "nlohmann/json.hpp"
#include <cstdint>
#include <exception>
#include <fstream>
#include <memory>
#include <string>
#include <utility>

namespace IO::AnimationIO {

    using json = nlohmann::json;

    json AnimationToJson(Animation::SpriteAnimationSet &animation) {
        json data;

        if (!animation.sheet || !Animation::ValidateSet(animation)) {
            LOG_ERROR("SpriteSerialization", "Could not validate sprite animation set.");
        } else {

            auto &resources = Core::ResourceManager::GetInstance();
            const auto &sheet = *animation.sheet;
            const auto textureID = resources.GetKey(sheet.texture);
            if (textureID.empty()) {
                LOG_ERROR("Animation Serialization", "Animation texture is not registered.");
                return nullptr;
            }
            data = {{"sheet",
                            {

                                    {"texture_id", textureID},

                                    {"columns", sheet.columns},

                                    {"rows", sheet.rows},

                                    {"column_spacing", sheet.columnSpacing},

                                    {"row_spacing", sheet.rowSpacing}

                            }},
                    {"clips", json::object()}};

            for (const auto &[name, clip] : animation.clips) {
                auto frames = json::array();

                for (const auto &frame : clip.frames) {

                    frames.push_back({{"index", frame.index}, {"duration", frame.duration}});
                }

                data["clips"][name] = {{"loop", clip.loop}, {"frames", std::move(frames)}};
            }
        }
        return data;
    }

    Animation::SpriteAnimationSet JsonToAnimation(const nlohmann::json &data) {

        Animation::SpriteAnimationSet set{};
        auto &resources = Core::ResourceManager::GetInstance();

        if (data.contains("sheet")) {
            set.sheet = std::make_shared<Rendering::SpriteSheet>();
            auto &sheet = *set.sheet;
            const auto &sheetData = data["sheet"];

            if (sheetData.contains("texture_id")) {
                sheet.texture = resources.Get<Rendering::Texture>(sheetData["texture_id"].get<std::string>());
            }

            sheet.columns = sheetData.value("columns", 1);
            sheet.rows = sheetData.value("rows", 1);
            sheet.columnSpacing = sheetData.value("column_spacing", 0);
            sheet.rowSpacing = sheetData.value("row_spacing", 0);

            if (data.contains("clips")) {

                for (const auto &[name, clipData] : data["clips"].items()) {
                    Animation::SpriteClip clip;
                    clip.loop = clipData.value("loop", true);

                    if (clipData.contains("frames")) {
                        for (const auto &framedata : clipData["frames"]) {
                            Animation::SpriteFrame frame;
                            frame.index = framedata.value("index", uint32_t{0});
                            frame.duration = framedata.value("duration", 0.1f);
                            clip.frames.push_back(frame);
                        }
                    }
                    set.clips.emplace(name, std::move(clip));
                }
            }
            return set;
        }
        return set;
    }

    bool Serialize(Animation::SpriteAnimationSet &set, const std::filesystem::path &path) {
        try {
            const json data = AnimationToJson(set);
            if (data.is_null()) {
                return false;
            }

            const std::string text = data.dump(4);
            const auto resolved = VFS::Resolve(path);
            if (resolved.empty()) {
                LOG_ERROR("Animation Serialization", "Could not resolve path: " + path.string());
                return false;
            }

            std::ofstream out(resolved);
            if (!out.is_open()) {
                LOG_ERROR("Animation Serialization", "Could not open path for serialization: " + path.string());
                return false;
            }

            out << text;
            out.close();
            if (!out) {
                LOG_ERROR("Animation Serialization", "Failed to write path: " + path.string());
                return false;
            }
            return true;
        } catch (const std::exception &e) {
            LOG_ERROR("Animation Serialization", "Failed to serialize " + path.string() + ": " + e.what());
            return false;
        }
    }

    Animation::SpriteAnimationSet Deserialize(const std::filesystem::path &path) {
        try {
            const auto data = VFS::ReadVirtualJson(path);

            if (!data.has_value()) {
                return {};
            }

            auto set = JsonToAnimation(*data);

            if (!set.sheet || !Animation::ValidateSet(set)) {
                LOG_ERROR("Animation Serialization", "Invalid animation set: " + path.string());
                return {};
            }

            return set;
        } catch (const std::exception &e) {
            LOG_ERROR("Animation Serialization", "Failed to deserialize " + path.string() + ": " + e.what());
            return {};
        }
    }
} // namespace IO::AnimationIO
