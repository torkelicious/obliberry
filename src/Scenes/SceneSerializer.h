#ifndef OBLIBERRY_SCENESERIALIZER_H
#define OBLIBERRY_SCENESERIALIZER_H
#include "Scene.h"

// outline..
namespace SceneIO {
    class SceneSerializer {
    public:
        /*
         * JSON or YAML style format probably?, text based atleast.
         * scene files (.oblvl?) will hold:
         * metadata:
         * name, cam pos, etc whateber is added later
         *
         * A reference to .obmap file to load map (via MapIO)
         *
         * ECS Registry
         * each entity declaration holds:
         *  id, components list, its own metadata maybe?
         *
         *TODO: implement...
         */

        SceneSerializer(Scene *scene) : m_Scene(scene) {
        }

        bool Serialize(const std::string &filepath, Scene &scene);

        bool Deserialize(const std::string &filepath, Scene &scene);

    private:
        Scene *m_Scene;
    };
}

#endif //OBLIBERRY_SCENESERIALIZER_H
