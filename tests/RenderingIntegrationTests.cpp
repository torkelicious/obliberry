#include "Rendering/Types/FBO/FrameBuffer.h"
#include "Rendering/Types/Shader/Shader.h"
#include "Rendering/PostProcessing/PostProcessing.h"
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <gtest/gtest.h>
#include <array>
#include <cstdlib>
#include <memory>

class RenderingIntegrationTests : public testing::Test {
protected:
    GLFWwindow *window = nullptr;
    void SetUp() override {
        const bool nullPlatform = std::getenv("OBLIBERRY_TEST_NULL_GLFW") != nullptr;
        if (nullPlatform) {
            glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_NULL);
        }
        if (!glfwInit()) {
            ASSERT_EQ(std::getenv("OBLIBERRY_REQUIRE_RENDERING_TESTS"), nullptr)
                << "CI requires a working display for rendering tests.";
            GTEST_SKIP() << "GLFW could not initialize a display.";
        }
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        if (nullPlatform) {
            glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_OSMESA_CONTEXT_API);
        }
        window = glfwCreateWindow(32, 32, "Rendering tests", nullptr, nullptr);
        if (!window) {
            ASSERT_EQ(std::getenv("OBLIBERRY_REQUIRE_RENDERING_TESTS"), nullptr)
                << "CI requires an OpenGL 4.3 context.";
            GTEST_SKIP() << "An OpenGL 4.3 context is unavailable.";
        }
        glfwMakeContextCurrent(window);
        ASSERT_TRUE(gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)));
    }
    void TearDown() override {
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }
};

TEST_F(RenderingIntegrationTests, PickingReadsRenderedEntityAndBackground) {
    Rendering::FrameBuffer framebuffer{16, 16, true};
    framebuffer.BindDrawBuffers();
    framebuffer.ClearEntityIDAttachment(-1);
    framebuffer.Bind();
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    Rendering::Shader shader{
        R"glsl(#version 430 core
            void main() {
                vec2 p[3] = vec2[3](vec2(-0.5,-0.5), vec2(0.5,-0.5), vec2(0,0.5));
                gl_Position = vec4(p[gl_VertexID], 0, 1);
            })glsl",
        R"glsl(#version 430 core
            layout(location=0) out vec4 color;
            layout(location=1) out int entity;
            void main() { color=vec4(1); entity=42; })glsl",
        "Picking test"
    };
    shader.InitGL();
    ASSERT_TRUE(shader.IsValid());
    shader.Bind();
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vao);
    EXPECT_EQ(framebuffer.ReadEntityID(8, 8), 42);
    EXPECT_EQ(framebuffer.ReadEntityID(0, 0), -1);
    EXPECT_EQ(framebuffer.ReadEntityID(16, 0), -1);
    EXPECT_EQ(glGetError(), static_cast<GLenum>(GL_NO_ERROR));
}

TEST_F(RenderingIntegrationTests, ResizingRecreatesCompleteAttachments) {
    Rendering::FrameBuffer framebuffer{8, 8, true};
    framebuffer.Invalidate(23, 17);
    EXPECT_EQ(framebuffer.GetWidth(), 23u);
    EXPECT_EQ(framebuffer.GetHeight(), 17u);
    framebuffer.BindDrawBuffers();
    framebuffer.Bind();
    EXPECT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), static_cast<GLenum>(GL_FRAMEBUFFER_COMPLETE));
    framebuffer.ClearEntityIDAttachment(73);
    EXPECT_EQ(framebuffer.ReadEntityID(22, 16), 73);
    EXPECT_EQ(framebuffer.ReadEntityID(23, 16), -1);
    EXPECT_EQ(glGetError(), static_cast<GLenum>(GL_NO_ERROR));
}

TEST_F(RenderingIntegrationTests, EffectsExecuteInOrderAndSkipDisabledPasses) {
    const std::string vertex = R"glsl(#version 430 core
        out vec2 uv;
        void main() {
            vec2 p=vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
            uv=p; gl_Position=vec4(p*2-1,0,1);
        })glsl";
    auto add = std::make_shared<Rendering::Shader>(vertex, R"glsl(#version 430 core
        in vec2 uv; out vec4 color; uniform sampler2D u_Texture;
        void main() { color=texture(u_Texture,uv)+vec4(0.125,0,0,0); })glsl", "Add test");
    auto multiply = std::make_shared<Rendering::Shader>(vertex, R"glsl(#version 430 core
        in vec2 uv; out vec4 color; uniform sampler2D u_Texture;
        void main() { color=texture(u_Texture,uv)*vec4(2,1,1,1); })glsl", "Multiply test");
    add->InitGL();
    multiply->InitGL();
    ASSERT_TRUE(add->IsValid());
    ASSERT_TRUE(multiply->IsValid());
    Rendering::FrameBuffer scene{4, 4, false};
    Rendering::FrameBuffer pingA{4, 4, false};
    Rendering::FrameBuffer pingB{4, 4, false};
    scene.BindDrawBuffers();
    scene.Bind();
    const GLfloat original[] = {0.25f, 0, 0, 1};
    glClearBufferfv(GL_COLOR, 0, original);

    Rendering::PostProcessing::PostProcessor processor;
    Rendering::PostProcessing::PostEffect first;
    first.shader = add;
    Rendering::PostProcessing::PostEffect second;
    second.shader = multiply;
    processor.AddEffect(first);
    processor.AddEffect(second);
    const auto red = [&](Rendering::FrameBuffer *buffer) {
        glBindFramebuffer(GL_FRAMEBUFFER, buffer->GetID());
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        std::array<float, 4> pixel{};
        glReadPixels(1, 1, 1, 1, GL_RGBA, GL_FLOAT, pixel.data());
        return pixel[0];
    };
    EXPECT_NEAR(red(processor.Execute(&scene, &pingA, &pingB)), 0.75f, 0.01f);
    std::swap(processor.Effects()[0], processor.Effects()[1]);
    EXPECT_NEAR(red(processor.Execute(&scene, &pingA, &pingB)), 0.625f, 0.01f);
    processor.Effects()[1].enabled = false;
    EXPECT_NEAR(red(processor.Execute(&scene, &pingA, &pingB)), 0.5f, 0.01f);
    EXPECT_EQ(glGetError(), static_cast<GLenum>(GL_NO_ERROR));
}
