#pragma once


#include "Map/Hex.h"
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "Core/Utils.h"
#include "Rendering/Mesh.h"
#include "Rendering/Material.h"
#include "Rendering/Lightmap.h"
#include "Math/Math.h"

namespace ECS::Components {
    struct MapComponent {
        Map::HexGrid grid;
        std::string mapFilePath = Core::PathUtils::Join(Core::MAP_PATH, "default", Core::MAP_FILE_EXTENSION);

        // visual assets
        std::shared_ptr<Rendering::Mesh> hexMesh;
        std::unordered_map<uint8_t, Rendering::Material> typeMats;
        std::shared_ptr<Rendering::Material> outlineMat;
        std::shared_ptr<Rendering::Material> pathToMat;

        // monobuffers
        //               // type            transforms
        std::unordered_map<uint8_t, std::vector<glm::mat4>> visibles;

        // padding bounds
        Math::Projection::AABB bufferedRenderAABB;

        // lighting
        Rendering::Lightmap lightmap;

        bool needsMeshUpdate = true;
    };
} // namespace ECS::Components
