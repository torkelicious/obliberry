#ifndef OBLIBERRY_MAPCOMPONENT_H
#define OBLIBERRY_MAPCOMPONENT_H
#include "Map/Hex.h"
#include <memory>
#include <string>

#include "Core/Utils.h"

struct MapComponent {
    HexGrid grid;
    std::string mapFilePath = PathUtils::Join(SCENE_PATH, "default.obmap");
    // Visual assets
    std::shared_ptr<Mesh> hexMesh;
    Material grassMat;
    Material sandMat;
    Material outlineMat;
    Material pathToMat;
};

#endif //OBLIBERRY_MAPCOMPONENT_H