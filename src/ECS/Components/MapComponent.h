#ifndef OBLIBERRY_MAPCOMPONENT_H
#define OBLIBERRY_MAPCOMPONENT_H

#include "Map/Hex.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "Core/Utils.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material.h"
#include "Math/Math.h"

struct MapChunk {
    Math::Projection::AABB bounds;
    std::vector<glm::mat4> grassTransforms;
    std::vector<glm::mat4> sandTransforms;
};

struct MapComponent {
    HexGrid grid;
    std::string mapFilePath = PathUtils::Join(MAP_PATH, "default", MAP_FILE_EXTENSION);

    // visual assets
    std::shared_ptr<Mesh> hexMesh;

    std::shared_ptr<Material> grassMat;
    std::shared_ptr<Material> sandMat;
    std::shared_ptr<Material> outlineMat;
    std::shared_ptr<Material> pathToMat;

    // chunked instanced transforms
    std::unordered_map<glm::ivec2, MapChunk> chunks;
    bool needsMeshUpdate = true;

    // batching
    std::vector<glm::mat4> activeGrass;
    std::vector<glm::mat4> activeSand;
    Math::Projection::AABB lastViewBounds;
};

#endif //OBLIBERRY_MAPCOMPONENT_H
