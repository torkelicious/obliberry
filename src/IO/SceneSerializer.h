#ifndef OBLIBERRY_SCENESERIALIZER_H
#define OBLIBERRY_SCENESERIALIZER_H
#include <string>

#include <string>

class Scene;

namespace SceneIO {
    bool Deserialize(const std::string &path, Scene &scene);

    bool Serialize(const std::string &path, Scene &scene);
}

#endif //OBLIBERRY_SCENESERIALIZER_H
