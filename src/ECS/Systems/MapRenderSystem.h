#ifndef OBLIBERRY_MAPRENDERSYSTEM_H
#define OBLIBERRY_MAPRENDERSYSTEM_H

#include "ECS/ECS.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "Renderer/Renderer.h"

namespace MapRenderSystem {
    inline void Render(Registry &registry, Renderer &renderer) {
        registry.ForEach<MapComponent, MapStateComponent>(
            [&](Entity entity, MapComponent *mapComp, MapStateComponent *stateComp) {
                // draw base tiles
                for (const auto &[pos, tile]: mapComp->grid.tiles) {
                    const Material &mat = (tile.type == TileType::Grass) ? mapComp->grassMat : mapComp->sandMat;
                    glm::vec2 worldPos = mapComp->grid.GetWorldPos(pos);
                    Transform t;
                    t.SetPosition({worldPos.x, worldPos.y, 0.0f});
                    renderer.Submit(*mapComp->hexMesh, mat, t);
                }

                // mouse selection outline
                if (stateComp->hasSelection) {
                    glm::vec2 worldPos = mapComp->grid.GetWorldPos(stateComp->selectedHex);
                    Transform t;
                    t.SetPosition({worldPos.x, worldPos.y, 0.01f});
                    t.SetScale({1.08f, 1.08f, 1.0f});
                    renderer.Submit(*mapComp->hexMesh, mapComp->outlineMat, t);
                }

                // movement destination highlight
                if (stateComp->hasPathTo) {
                    glm::vec2 worldPos = mapComp->grid.GetWorldPos(stateComp->pathTo);
                    Transform t;
                    t.SetPosition({worldPos.x, worldPos.y, 0.01f});
                    t.SetScale({1.08f, 1.08f, 1.0f});
                    renderer.Submit(*mapComp->hexMesh, mapComp->pathToMat, t);
                }
            }
        );
    }
}

#endif //OBLIBERRY_MAPRENDERSYSTEM_H
