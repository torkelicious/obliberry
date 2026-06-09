#ifndef OBLIBERRY_MESHCOMPONENT_H
#define OBLIBERRY_MESHCOMPONENT_H
#include <memory>
class Mesh;

// just a wrapper for now
struct MeshComponent {
    std::shared_ptr<Mesh> mesh;
};

#endif //OBLIBERRY_MESHCOMPONENT_H
