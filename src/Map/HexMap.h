

#ifndef ISOMETRICGAME_HEXMAP_H
#define ISOMETRICGAME_HEXMAP_H
#include <vector>
#include <glm/glm.hpp>

struct HexTile {
    int q = 0;
    int r = 0;
    glm::vec3 WorldPos = {0.0f, 0.0f, 0.0f};
    //TileType type;
};

class HexMap {
public:
    static constexpr float HEX_SPACING = 25.0f;
    std::vector<HexTile> tiles;

    glm::vec3 HexToWorld(int q, int r);

    void Generate(int radius);
};


#endif //ISOMETRICGAME_HEXMAP_H
