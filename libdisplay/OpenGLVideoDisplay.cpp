#include <GLES2/gl2.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>
#include <atomic>
#include <array>
#include <utility>
#include "IVideoDisplay.h"
#include "log.h"
#include "opengl/Shader.h"
#include "opengl/Mesh.h"

#ifndef __ANDROID__
#include <GLFW/glfw3.h>

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

    auto should_close() -> bool override
    {
        return glfwWindowShouldClose(_handle);
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
#endif

static const char *img_vert_src = R"(
#version 100
attribute vec2 pos;
attribute vec2 uv;
attribute vec2 idx;
varying vec2 uv_;
uniform ivec2 dims;
void main()
{\n\
    uv_ = uv;
    gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);
}
)";

static const char *img_frag_src = R"(
#version 100
precision mediump float;
uniform sampler2D img;
uniform ivec2 dims;
uniform int hinv;
varying vec2 uv_;
void main()
{
    float screen_hwratio = float(dims.y) / float(dims.x);
    float video_whratio = 1.333333;
    float u_max = screen_hwratio / video_whratio;
    vec2 uv = uv_.yx * vec2(u_max, 1.0);
    float free_u = u_max - 1.0;
    uv.x -= free_u / 2.0;
    if (uv.x < 0.0 || uv.x > 1.0) {
        gl_FragColor = vec4(vec3(0.0), 1.0);
    } else {
        if (hinv > 0) {
            uv.y = (1.0 - uv.y);
        }
        vec4 s = texture2D(img, uv);
        gl_FragColor = vec4(vec3(s.a), 1.0);
    }
}
)";

class OpenGLVideoDisplay : public IVideoDisplay
{
public:
    OpenGLVideoDisplay(IWindow *window):
        _window(window),
        _img_mesh(_img_shader),
        _next_frame(nullptr)
    {
        _img_shader.load(img_vert_src, img_frag_src);
        _img_shader.configure({"pos", "uv"}, {"dims", "hinv"});
        _img_shader.add_texture("img");

        _img_mesh.make_rect(-1.0f, -1.0f, 2.0f, 2.0f);

        glClearColor(0.3, 0.0, 0.3, 1.0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    virtual auto open() -> void override {}
    virtual auto set_video_frame(const VideoFramePtr&) -> void override;
    virtual auto update() -> bool override;

private:
    IWindow *_window;

    std::atomic<VideoFrame*> _next_frame;
    Shader _img_shader;
    Mesh _img_mesh;
};

auto OpenGLVideoDisplay::set_video_frame(const VideoFramePtr& frame) -> void
{
    const auto &fmt = frame->format;

#if 0
    _img_shader.update_texture("img", fmt.width, fmt.height, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, frame->buffer.data());
#else
    // GLES2.0 only supports GL_LUMINANCE and GL_ALPHA with GL_UNSIGNED_BYTE, hence need to rescale the values

    uint8_t vmin{255}, vmax{0};

    for (size_t i = 0; i < frame->format.width * frame->format.height; i++) {
        uint16_t a;
        memcpy(&a, frame->buffer.data() + (i * sizeof a), sizeof a);

        uint8_t b = a / 256;
        memcpy(frame->buffer.data() + i, &b, sizeof b);

        vmin = std::min(vmin, b);
        vmax = std::max(vmax, b);
    }

    INFO("Post conversion (%d min) (%d max)\n", vmin, vmax);

    auto *next_frame = new VideoFrame(std::move(*frame));
    _next_frame.store(next_frame);
#endif
}

auto OpenGLVideoDisplay::update() -> bool
{
    auto [w, h] = _window->get_size();

    _window->poll();

    if (VideoFrame *frame = _next_frame.exchange(nullptr); frame) {
        auto &fmt = frame->format;
        _img_shader.update_texture("img", fmt.width, fmt.height, GL_ALPHA, GL_ALPHA, GL_UNSIGNED_BYTE, frame->buffer.data());

        delete frame;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    _img_shader.use();
    _img_shader.update_uniform("dims", std::array{w, h});
    _img_shader.update_uniform("hinv", 0);
    _img_mesh.draw();

    _window->swap_buffers();

    return !_window->should_close();
}

#ifndef __ANDROID__
auto create_glfw_video_display(int width, int height) -> IVideoDisplay*
{
    return new OpenGLVideoDisplay(new GlfwWindow(width, height));
}
#endif

auto create_user_window_video_display(IWindow *window) -> IVideoDisplay*
{
    return new OpenGLVideoDisplay(window);
}
