#pragma once

#include <memory>
class Mesh;

struct MeshComponent {
    std::shared_ptr<Mesh> mesh;
};
