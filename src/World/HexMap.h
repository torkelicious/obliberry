

#ifndef ISOMETRICGAME_HEXMAP_H
#define ISOMETRICGAME_HEXMAP_H
#include <glm/glm.hpp>

struct HexTile {
    int q;
    int r;
    glm::vec2 WorldPos;
    //TileType type;
};

class HexMap {
public:
    static constexpr float HEX_SPACING = 25.0f;
    std::vector<HexTile> tiles;

    void Generate(int radius);
};


#endif //ISOMETRICGAME_HEXMAP_H
