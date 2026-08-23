#pragma once

#include <filesystem>
#include <limits>
#include <string>
#include <string_view>

namespace Core {
    // Ammount of threads used by the Engine's most important functions
    // 2 for Main & Render
    // We do not reserve Logger since it's mostly idle in most cases. It can share.
    constexpr unsigned ReservedThreads = 2;

    // filepaths (vfs relative)
    constexpr std::string_view ASSET_PATH = "assets/";
    constexpr std::string_view SHADER_PATH = "assets/shaders/";
    constexpr std::string_view TEXTURE_PATH = "assets/textures/";
    constexpr std::string_view MAP_PATH = "assets/maps/";
    constexpr std::string_view SCENE_PATH = "assets/scenes/";
    constexpr std::string_view SCRIPT_PATH = "assets/scripts/";
    constexpr std::string_view AUDIO_PATH = "assets/audio/";
    constexpr std::string_view PREFAB_PATH = "assets/prefabs/";
    constexpr std::string_view PARTICLE_PRESET_PATH = "assets/particle_presets/";
    constexpr std::string_view FONT_PATH = "assets/fonts/";

    // editor executable-relative
    constexpr std::string_view E_RESOURCES_PATH = "internal/resources/";
    constexpr std::string_view E_EDITOR_FONTS_PATH = "internal/resources/fonts/";

    // fs extensions
    constexpr std::string_view MAP_FILE_EXTENSION = ".obmap";
    constexpr std::string_view SCENE_FILE_EXTENSION = ".json";
    constexpr std::string_view SCRIPT_FILE_EXTENSION = ".obsl";
    constexpr std::string_view PACKAGE_FILE_EXTENSION = ".obpak";
    // misc file stuff
    constexpr std::string_view MAP_FILE_MAGIC_STR = "OBLIHEXM";
    constexpr uint16_t MAP_FILE_VERSION = 2;
    // window config
    constexpr int WINDOW_WIDTH = 1280;
    constexpr int WINDOW_HEIGHT = 720;
    constexpr float TARGET_ASPECT = 16.0f / 9.0f;
    // self explanitory
    constexpr float ZOOM_SPEED = 0.2f;
    constexpr float PAN_SPEED = 12.f;

    // hex stuff
    // point top hexes
    constexpr float HEX_SIZE = 0.5f;
    constexpr std::size_t HEX_NEIGHBOR_COUNT = 6;
    // matrix math stuff
    constexpr float HEX_INV_MAT_Q_X = 0.577350269f;  // sqrt(3) / 3
    constexpr float HEX_INV_MAT_Q_Y = -0.333333333f; // -1.0 / 3.0
    constexpr float HEX_INV_MAT_R_Y = 0.666666667f;  // 2.0 / 3.0
    // coord math stuff
    constexpr float HEX_HEIGHT_MULTIPLIER = 2.0f;     // total height is 2 * size
    constexpr float HEX_HEIGHT_SPACING_RATIO = 0.75f; // vertical step is 75% of total height
    constexpr float HEX_ODD_ROW_OFFSET = 0.5f;
    constexpr int P_INFINITY = std::numeric_limits<int>::max() / 2;
    // map rendering
    constexpr float CHUNK_SIZE = 20.0f;
    // misc math
    constexpr float PI = 3.14159265359f;
    constexpr float SQRT_3 = 1.7320508075688772f;

    // lighting
    constexpr int LIGHTMAP_TEXELS_PER_HEX = 2;
} // namespace Core
