#pragma once

#include "Mesh.h"

namespace Rendering::MeshFactory {
    MeshData CreateQuad();

    MeshData CreatePointTopHex(float size = 0.5f);

    MeshData CreateEquiTriangle(float height);

    MeshData CreateEllipse(
        float radX = 0.5,
        float radY = 0.5,
        unsigned int segments = 50
    );

    MeshData CreateRegularPolygon(
        unsigned int sides = 3,
        float radius = 0.5);

    MeshData CreateRing(
        float innerRadius = 0.25,
        float outerRadius = 0.5,
        unsigned int segments = 50);

    MeshData CreateSector(
        float radius = 0.5,
        float startAngle = 0,
        float endAngle = 90,
        unsigned int segments = 50);

    MeshData CreateDiamond(
        float width = 0.5,
        float height = 0.5);

    void AppendMesh(MeshData &dst, const MeshData &src);

    void RegisterAllMeshFactories();
} // namespace Rendering::MeshFactory
