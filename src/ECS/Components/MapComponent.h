#ifndef OBLIBERRY_MAPCOMPONENT_H
#define OBLIBERRY_MAPCOMPONENT_H
#include "Map/Hex.h"
#include <memory>
#include <string>

#include "Core/Utils.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material.h"

struct MapComponent {
    HexGrid grid;
    std::string mapFilePath = PathUtils::Join(MAP_PATH, "default", MAP_FILE_EXTENSION);
    // visual assets
    std::shared_ptr<Mesh> hexMesh;
    Material grassMat;
    Material sandMat;
    Material outlineMat;
    Material pathToMat;

    // batched meshes
    std::shared_ptr<Mesh> grassMesh;
    std::shared_ptr<Mesh> sandMesh;
    bool needsMeshUpdate = true;
};

#endif //OBLIBERRY_MAPCOMPONENT_H
