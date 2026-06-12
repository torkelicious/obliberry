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
#include "Renderer/Lightmap.h"
#include "Math/Math.h"

struct MapComponent {
    HexGrid grid;
    std::string mapFilePath = PathUtils::Join(MAP_PATH, "default", MAP_FILE_EXTENSION);

    // visual assets
    std::shared_ptr<Mesh> hexMesh;
    std::unordered_map<uint8_t, Material> typeMats;
    std::shared_ptr<Material> outlineMat;
    std::shared_ptr<Material> pathToMat;

    bool needsMeshUpdate = true;

    // monobuffers
    //               // type            transforms
    std::unordered_map<uint8_t, std::vector<glm::mat4> > visibles;

    // padding bounds
    Math::Projection::AABB bufferedRenderAABB;

    // lighting
    Lightmap lightmap;
};

#endif //OBLIBERRY_MAPCOMPONENT_H
