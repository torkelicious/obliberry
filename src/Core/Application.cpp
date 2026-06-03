#include "Application.h"
#include "Graphics/Camera.h"
#include "Graphics/Mesh.h"
#include "Graphics/MeshFactory.h"
#include "Graphics/Renderer.h"
#include "Graphics/Shader.h"
#include "Graphics/Texture.h"
#include "Graphics/GLDebug.h"
#include "World/HexMap.h"

Application::Application()
    : m_Window(1280, 720, "Window", true) {
}

void Application::Run() {
    GLDebug::initDbg();
    // move this somewhere else
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Renderer renderer;

    Camera camera;
    m_Window.SetCamera(&camera);

    /*
     * later on objects will work via components
     * i.e MapHex has a Hex Mesh, Transform, Texture
     * all mapped in its own Entity(?) class
     */

    Mesh hexMesh = MeshFactory::CreatePointTopHex(HexMap::HEX_SPACING);
    Shader shader("assets/shaders/basic.vert", "assets/shaders/basic.frag");

    shader.Bind();
    HexMap map;
    map.Generate(10);

    Texture tex("assets/textures/uv_grid.jpg", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST);
    tex.Bind();
    shader.SetUniform1i("u_Texture", 0);

    Material mat;
    mat.shader = &shader;
    //mat.texture = &tex;
    mat.color = {1, 1, 1, 1};


    camera.target = {0.0f, 0.0f, 0.0f};
    camera.offset = {0, 50, 50};

    while (!m_Window.ShouldClose()) {
        renderer.BeginFrame(camera);

        // Render tiles
        for (const auto &tile: map.tiles) {
            Transform t;
            t.Position = tile.WorldPos;
            t.Scale = {1.0f, 1.0f};
            t.rotation = 0;
            renderer.Submit(hexMesh, mat, t);
        }

        // end frame
        renderer.Flush();

        m_Window.SwapBuffers();
        m_Window.PollEvents();
    }
}
