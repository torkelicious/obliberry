#include "Application.h"

#include "Graphics/Mesh.h"
#include "Graphics/Renderer.h"
#include "Graphics/Shader.h"

Application::Application()
    :m_Window(1280,720,"Window")
{
}

MeshData CreateQaud() {
    MeshData mesh;

    mesh.vertices =
      {
            {0,0, 0,0},
            {1,0, 1,0},
            {1,1, 1,1},
            {0,1, 0,1}
      };

    mesh.indices = {0,1,2,2,3,0};
    return mesh;
}

void Application::Run()
{
    Renderer renderer;

    Mesh quad;
    quad.Upload(CreateQaud());

    Transform t;
    t.Position = { 100.0f, 200.0f };
    t.Scale    = { 64.0f, 64.0f };

    Shader shader(
        "assets/shaders/basic.vert",
        "assets/shaders/basic.frag"
        );
    shader.Bind();

    // temp
    glm::mat4 vp = glm::ortho(0.0f, 1280.0f, 0.0f, 720.0f, -1.0f, 1.0f);
    shader.SetUniformMat4("u_ViewProjection", vp);

    while (!m_Window.ShouldClose())
    {

        t.Position.x += 1.0f;
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        renderer.Draw(quad,shader,t);

        m_Window.SwapBuffers();
        m_Window.PollEvents();
    }
}
