#include "Application.h"
#include "Graphics/Camera.h"
#include "Graphics/Mesh.h"
#include "Graphics/MeshFactory.h"
#include "Graphics/Renderer.h"
#include "Graphics/Shader.h"
#include "Graphics/Texture.h"
#include <glm/glm.hpp>

Application::Application()
    : m_Window(1280, 720, "Window")
{
}

void Application::Run()
{
    Renderer renderer;

    Camera camera;
    m_Window.SetCamera(&camera);

    // Example object
    Mesh hex = MeshFactory::CreatePointTopHex(65);

    Transform t;
    t.Position = {0.0f, 0.0f};
    t.Scale = {1.0f, 1.0f};
    t.rotation = 0.0f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shader shader("assets/shaders/basic.vert", "assets/shaders/basic.frag");

    Texture tex("assets/textures/uv_grid.jpg");
    tex.Bind();

    shader.SetUniform1i("u_Texture", 0);

    camera.target = {0.0f, 0.0f, 0.0f};

    float angle = 0.0f;

    while (!m_Window.ShouldClose())
    {
        angle += 0.01f;

        float radius = 500.0f;

        glm::vec3 offset;
        offset.x = sin(angle) * radius;
        offset.y = 500.0f;
        offset.z = cos(angle) * radius;

        camera.offset = offset;

        glm::mat4 vp = camera.GetVP();

        renderer.Clear();
        shader.Bind();
        renderer.Draw(hex, shader, t, vp);

        m_Window.SwapBuffers();
        m_Window.PollEvents();
    }

}