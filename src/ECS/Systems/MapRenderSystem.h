#ifndef OBLIBERRY_MAPRENDERSYSTEM_H
#define OBLIBERRY_MAPRENDERSYSTEM_H
#include "Map/Hex.h"
#include "Map/HexCoords.h"
#include "Renderer/Renderer.h"

namespace MapRenderSystem {
    inline void Render(Renderer &renderer, const HexGrid &grid,
                       const Material &grassMat, const Material &sandMat,
                       const Material &outlineMat, const Material &pathToMat,
                       std::shared_ptr<Mesh> hexMesh,
                       const HexCoords &selectedHex, bool hasSelection,
                       const HexCoords &pathTo, bool hasPathTo) {
        // Draw base tiles
        for (const auto &[pos, tile]: grid.tiles) {
            const Material &mat = (tile.type == TileType::Grass) ? grassMat : sandMat;
            glm::vec2 worldPos = grid.GetWorldPos(pos);
            Transform t;
            t.SetPosition({worldPos.x, worldPos.y, 0.0f});
            renderer.Submit(*hexMesh, mat, t);
        }

        // Draw active mouse selection outline
        if (hasSelection) {
            glm::vec2 worldPos = grid.GetWorldPos(selectedHex);
            Transform t;
            t.SetPosition({worldPos.x, worldPos.y, 0.01f});
            t.SetScale({1.08f, 1.08f, 1.0f});
            renderer.Submit(*hexMesh, outlineMat, t);
        }

        // Draw movement destination path highlight
        if (hasPathTo) {
            glm::vec2 worldPos = grid.GetWorldPos(pathTo);
            Transform t;
            t.SetPosition({worldPos.x, worldPos.y, 0.01f});
            t.SetScale({1.08f, 1.08f, 1.0f});
            renderer.Submit(*hexMesh, pathToMat, t);
        }
    }
}


#endif //OBLIBERRY_MAPRENDERSYSTEM_H
