
#include "Core/Window.h"
#include <iostream>

int main() {
    Window window(800, 600, "Window");
    while (!window.ShouldClose()) {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        window.SwapBuffers();
        window.PollEvents();
    }
}
