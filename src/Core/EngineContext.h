#pragma once


#include <string>

class SceneManager;
class AudioEngine;
class Window;
class InputManager;
class ResourceManager;
class Renderer;
class Camera;

namespace ObSL {
    class Interpreter;
}

struct EngineContext {
    std::string pendingScenePath;
    std::string startScenePath;
    Window *window = nullptr;
    InputManager *input = nullptr;
    ResourceManager *resources = nullptr;
    SceneManager *sceneManager = nullptr;
    Renderer *renderer = nullptr;
    Camera *camera = nullptr;
    ObSL::Interpreter *scriptEngine = nullptr;
    AudioEngine *audioEngine = nullptr;
    float deltaTime = 0.0f;
};
