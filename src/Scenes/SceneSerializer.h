#ifndef OBLIBERRY_SCENESERIALIZER_H
#define OBLIBERRY_SCENESERIALIZER_H
#include "Scene.h"

// outline..
namespace SceneIO {
    class SceneSerializer {
    public:
        SceneSerializer(Scene *scene) : m_Scene(scene) {
        }

        bool Serialize(const std::string &filepath, Scene &scene);

        bool Deserialize(const std::string &filepath, Scene &scene);

    private:
        Scene *m_Scene;
    };
}

#endif //OBLIBERRY_SCENESERIALIZER_H
