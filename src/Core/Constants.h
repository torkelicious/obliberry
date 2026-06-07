#ifndef OBLIBERRY_CONSTANTS_H
#define OBLIBERRY_CONSTANTS_H
#include <string>

// filepaths
const static std::string ASSET_PATH = "assets/";
const static std::string SHADER_PATH = ASSET_PATH + "shaders/";
const static std::string TEXTURE_PATH = ASSET_PATH + "textures/";
// window config
static constexpr int WINDOW_WIDTH = 1280;
static constexpr int WINDOW_HEIGHT = 720;
constexpr float TARGET_ASPECT = 16.0f / 9.0f;
// self explanitory
constexpr float ZOOM_SPEED = 0.2f;
constexpr float PAN_SPEED = 12.f;

// point top hexes
constexpr float HEX_SIZE = 0.5f;
constexpr std::size_t HEX_NEIGHBOR_COUNT = 6;
// matrix math stuff
constexpr float HEX_INV_MAT_Q_X = 0.577350269f; // sqrt(3) / 3
constexpr float HEX_INV_MAT_Q_Y = -0.333333333f; // -1.0 / 3.0
constexpr float HEX_INV_MAT_R_Y = 0.666666667f; // 2.0 / 3.0
// coord math stuff
constexpr float HEX_HEIGHT_MULTIPLIER = 2.0f; // total height is 2 * size
constexpr float HEX_HEIGHT_SPACING_RATIO = 0.75f; // vertical step is 75% of total height
constexpr float HEX_RADIUS_SPACING_MULTIPLIER = 1.5f; // vertical step is 1.5 * size
constexpr float HEX_ODD_ROW_OFFSET = 0.5f;
// big int for pathfinding :)
constexpr int P_INFINITY = 9999999;

#endif //OBLIBERRY_CONSTANTS_H
