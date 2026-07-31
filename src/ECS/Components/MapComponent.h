#pragma once


#include "Map/Hex.h"
#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "Core/Utils/PathUtils.h"
#include "Rendering/Mesh.h"
#include "Rendering/Material.h"
#include "Rendering/Lightmap.h"
#include "Math/Math.h"

namespace ECS::Components {
    struct MapComponent {
        // double buffered for zero copy thread safe submission
        std::array<std::array<std::vector<glm::mat4>, 256>, 2> visibles;

        // lighting
        Rendering::Lightmap lightmap;

        Map::HexGrid grid;

        std::array<std::vector<uint8_t>, 2> activeVisibleTypes; // keep track of which types have data

        std::string mapFilePath;

        // visual assets
        std::vector<std::pair<uint8_t, std::shared_ptr<Rendering::Material>>> typeMats;
        std::shared_ptr<Rendering::Mesh> hexMesh;
        std::shared_ptr<Rendering::Material> outlineMat;
        std::shared_ptr<Rendering::Material> pathToMat;

        // padding bounds
        Math::Projection::AABB bufferedRenderAABB;


        uint8_t activeBufferIndex = 0;

        bool needsMeshUpdate = true;
        bool mapDirty = false;

        // linear search on small collections
        [[nodiscard]] auto findTypeMat(uint8_t id) {
            return std::ranges::find_if(typeMats, [id](const auto &p) { return p.first == id; });
        }

        [[nodiscard]] auto findTypeMat(uint8_t id) const {
            return std::ranges::find_if(typeMats, [id](const auto &p) { return p.first == id; });
        }

        [[nodiscard]] std::vector<glm::mat4> &getVisibleTransforms(const uint8_t type) {
            // register this type on the current write buffer the first time
            // added a transform after a clear

            // reused vectors are still registered on the next rebuild
            if (visibles[activeBufferIndex][type].empty()) {
                activeVisibleTypes[activeBufferIndex].push_back(type);
            }
            return visibles[activeBufferIndex][type];
        }
    };
} // namespace ECS::Components
