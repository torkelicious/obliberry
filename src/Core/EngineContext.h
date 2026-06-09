#ifndef OBLIBERRY_ENGINECONTEXT_H
#define OBLIBERRY_ENGINECONTEXT_H

class Window;
class InputManager;
class ResourceManager;
class Camera;

struct EngineContext {
    Window *window = nullptr;
    InputManager *input = nullptr;
    ResourceManager *resources = nullptr;
    Camera *camera = nullptr;
    float deltaTime = 0.0f;
};


#endif //OBLIBERRY_ENGINECONTEXT_H
