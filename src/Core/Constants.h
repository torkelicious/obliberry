#ifndef OBLIBERRY_CONSTANTS_H
#define OBLIBERRY_CONSTANTS_H
#include <string>

const static std::string ASSET_PATH = "assets/";
const static std::string SHADER_PATH = ASSET_PATH + "shaders/";
const static std::string TEXTURE_PATH = ASSET_PATH + "textures/";
static constexpr int WINDOW_WIDTH = 1280;
static constexpr int WINDOW_HEIGHT = 720;
constexpr float TARGET_ASPECT = 16.0f / 9.0f;
constexpr float HEX_SIZE = 0.5f;
constexpr float ZOOM_SPEED = 0.2f;
constexpr float PAN_SPEED = 12.f;

#endif //OBLIBERRY_CONSTANTS_H
