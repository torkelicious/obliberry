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
    Window *window = nullptr;
    InputManager *input = nullptr;
    ResourceManager *resources = nullptr;
    SceneManager *sceneManager = nullptr;
    Renderer *renderer = nullptr;
    Camera *camera = nullptr;
    float deltaTime = 0.0f;
    ObSL::Interpreter *scriptEngine = nullptr;
    AudioEngine *audioEngine = nullptr;
    std::string pendingScenePath;
};
