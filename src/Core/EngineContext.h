#ifndef OBLIBERRY_ENGINECONTEXT_H
#define OBLIBERRY_ENGINECONTEXT_H


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
    Renderer *renderer = nullptr;
    Camera *camera = nullptr;
    float deltaTime = 0.0f;
    ObSL::Interpreter *scriptEngine = nullptr;
};


#endif //OBLIBERRY_ENGINECONTEXT_H
