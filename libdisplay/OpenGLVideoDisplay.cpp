#include "IVideoDisplay.h"
#include <GL/gl.h>
#include <GLFW/glfw3.h>

class IWindow
{
public:
    virtual auto resize(int width, int height) -> void = 0;
    virtual auto poll() -> void = 0;
    virtual auto should_close() -> void = 0;
    virtual auto swap_buffers() -> void = 0;
};

class GlfwWindow : public IWindow
{
public:
    GlfwWindow(int width = 1280, int height = 900)
    {
        glfwInit();
        _handle = glfwCreateWindow(width, height, "", nullptr, nullptr);
        glfwMakeContextCurrent(_handle);
    }

    auto poll() -> void override
    {
        glfwPollEvents();
    }


    auto should_close() -> void override
    {
        glfwWindowShouldClose(_handle);
    }

    auto resize(int width, int height) -> void override
    {
        glfwSetWindowSize(_handle, width, height);
    }

    auto swap_buffers() -> void override
    {
        glfwSwapBuffers(_handle);
    }

private:
    GLFWwindow *_handle;
};

class OpenGLVideoDisplay : public IVideoDisplay
{
public:
    OpenGLVideoDisplay(IWindow *window): _window(window)
    {
        glClearColor(0.0, 0.0, 0.0, 1.0);
    }

    virtual auto open() -> void override {}
    virtual auto set_video_frame(const VideoFramePtr&) -> void override;
    virtual auto update() -> void override;

private:
    IWindow *_window;
};

auto OpenGLVideoDisplay::set_video_frame(const VideoFramePtr& frame) -> void
{
}

auto OpenGLVideoDisplay::update() -> void
{
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    _window->swap_buffers();
}

auto create_glfw_video_display(int width, int height) -> IVideoDisplay*
{
    return new OpenGLVideoDisplay(new GlfwWindow(width, height));
}

