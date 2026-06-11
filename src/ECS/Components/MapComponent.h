#ifndef OBLIBERRY_MAPCOMPONENT_H
#define OBLIBERRY_MAPCOMPONENT_H

#include "Map/Hex.h"
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "Core/Utils.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material.h"
#include "Math/Math.h"

struct MapComponent {
    HexGrid grid;
    std::string mapFilePath = PathUtils::Join(MAP_PATH, "default", MAP_FILE_EXTENSION);

    // visual assets
    std::shared_ptr<Mesh> hexMesh;

    std::shared_ptr<Material> grassMat;
    std::shared_ptr<Material> sandMat;
    std::shared_ptr<Material> outlineMat;
    std::shared_ptr<Material> pathToMat;

    bool needsMeshUpdate = true;

    // zero alloc buffers
    std::vector<glm::mat4> visibleGrass;
    std::vector<glm::mat4> visibleSand;

    // padding bounds
    Math::Projection::AABB bufferedRenderAABB;
};

#endif //OBLIBERRY_MAPCOMPONENT_H
