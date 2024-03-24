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

class Shader
{
public:
    void load(const char *vert_src, const char *frag_src)
    {
        int vert_id = glCreateShader(GL_VERTEX_SHADER);
        int frag_id = glCreateShader(GL_FRAGMENT_SHADER);

        glShaderSource(vert_id, 1, &vert_src, 0);
        glCompileShader(vert_id);
        check_shader_status("vertex", vert_id);

        glShaderSource(frag_id, 1, &frag_src, 0);
        glCompileShader(frag_id);
        check_shader_status("fragment", frag_id);

        _id = glCreateProgram();
        glAttachShader(_id, vert_id);
        glAttachShader(_id, frag_id);
        glLinkProgram(_id);
    }

    void enable_attrib_vert(const std::string &name)
    {
        _vert_loc = glGetAttribLocation(_id, name.c_str());
    }

    void enable_attrib_texcoord(const std::string &name)
    {
        _texcoord_loc = glGetAttribLocation(_id, name.c_str());
    }

    void enable_uniform(const std::string &name)
    {
        _uniforms[name] = glGetUniformLocation(_id, name.c_str());
    }

    GLint get_attrib_vert() const
    {
        return _vert_loc;
    }

    GLint get_attrib_texcoord() const
    {
        return _texcoord_loc;
    }

    void add_texture(const std::string &name)
    {
        TexInfo &tex_info = _textures[name];

        glGenTextures(1, &tex_info.id);
        tex_info.uniform_loc = glGetUniformLocation(_id, name.c_str());
    }

    void update_texture(const std::string &name, GLsizei w, GLsizei h, GLint intern_fmt, GLenum data_fmt, GLenum data_type, const void *data)
    {
        const auto &tex_info = _textures.at(name);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_info.id);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, intern_fmt, w, h, 0, data_fmt, data_type, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    void update_uniform(const std::string &name, int value)
    {
        glUniform1i(_uniforms.at(name), value);
    }

    void update_uniform(const std::string &name, float value)
    {
        glUniform1f(_uniforms.at(name), value);
    }

    void update_uniform(const std::string &name, const std::array<int, 2> &values)
    {
        glUniform2i(_uniforms.at(name), values[0], values[1]);
    }

    void update_uniform(const std::string &name, const std::array<int, 4> &values)
    {
        glUniform4i(_uniforms.at(name), values[0], values[1], values[2], values[3]);
    }

    void update_uniform(const std::string &name, const std::array<float, 2> &values)
    {
        glUniform2f(_uniforms.at(name), values[0], values[1]);
    }

    void update_uniform(const std::string &name, const std::array<float, 4> &values)
    {
        glUniform4f(_uniforms.at(name), values[0], values[1], values[2], values[3]);
    }

    void use() const
    {
        glUseProgram(_id);

        int tex_slot = 0;

        for (const auto &[tex_name, tex_info] : _textures)
        {
            glActiveTexture(GL_TEXTURE0 + tex_slot);
            glBindTexture(GL_TEXTURE_2D, tex_info.id);
            glUniform1i(tex_info.uniform_loc, tex_slot);

            ++tex_slot;
        }
    }

private:
    int _id;
    GLint _vert_loc;
    GLint _texcoord_loc;

    struct TexInfo
    {
        GLuint id;
        GLint uniform_loc;
    };

    std::unordered_map<std::string, TexInfo> _textures;
    std::unordered_map<std::string, GLint> _uniforms;

    void check_shader_status(const char *name, int shader)
    {
        int ok;
        char err[256];

        glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);

        if (!ok)
        {
            glGetShaderInfoLog(shader, sizeof err, 0, err);
            ERROR("%s error: %s\n", name, err);
        }
    }
};

class Mesh
{
public:
    Mesh(const Shader &shader): _shader(shader)
    {
    }

    void make_rect(float x, float y, float w, float h)
    {
        _verts.resize(4 * 4);

        _verts[0] = x;
        _verts[1] = y;
        _verts[2] = 0.0f;
        _verts[3] = 1.0f;

        _verts[4] = x + w;
        _verts[5] = y;
        _verts[6] = 1.0f;
        _verts[7] = 1.0f;

        _verts[8] = x + w;
        _verts[9] = y + h;
        _verts[10] = 1.0f;
        _verts[11] = 0.0f;

        _verts[12] = x;
        _verts[13] = y + h;
        _verts[14] = 0.0f;
        _verts[15] = 0.0f;

        _indices.resize(6);

        _indices[0] = 0;
        _indices[1] = 3;
        _indices[2] = 2;

        _indices[3] = 2;
        _indices[4] = 1;
        _indices[5] = 0;

        glGenBuffers(1, &_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*4*4, _verts.data(), GL_STATIC_DRAW);

        GLint vert_loc = _shader.get_attrib_vert();
        glEnableVertexAttribArray(vert_loc);
        glVertexAttribPointer(vert_loc, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, (void*)0);
        GLint texcoord_loc = _shader.get_attrib_texcoord();
        glEnableVertexAttribArray(texcoord_loc);
        glVertexAttribPointer(texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, (void*)(sizeof(float)*2));

        glGenBuffers(1, &_ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short)*2*3, _indices.data(), GL_STATIC_DRAW);
    }

    void draw() const
    {
        glBindBuffer(GL_ARRAY_BUFFER, _vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
    }

private:
    const Shader &_shader;
    unsigned int _vbo, _ebo;

    std::vector<float> _verts;
    std::vector<unsigned short> _indices;
};

static const char *img_vert_src = "\
#version 100\n\
attribute vec2 a_vert;\n\
attribute vec2 a_texcoord;\n\
varying vec2 v_texcoord;\n\
uniform ivec2 dims;\n\
void main()\n\
{\n\
    v_texcoord = a_texcoord;\n\
    gl_Position = vec4(a_vert.x, a_vert.y, 0.0, 1.0);\n\
}\n\
";

static const char *img_frag_src = "\
#version 100\n\
precision mediump float;\n\
uniform sampler2D img;\n\
uniform ivec2 dims;\n\
uniform int hinv;\n\
varying vec2 v_texcoord;\n\
void main()\n\
{\n\
    float screen_hwratio = float(dims.y) / float(dims.x);\n\
    float video_whratio = 1.333333;\n\
    float u_max = screen_hwratio / video_whratio;\
    vec2 uv = v_texcoord.yx * vec2(u_max, 1.0);\
    float free_u = u_max - 1.0;\n\
    uv.x -= free_u / 2.0;\n\
    if (uv.x < 0.0 || uv.x > 1.0) {\
        gl_FragColor = vec4(vec3(0.0), 1.0);\n\
    } else {\n\
        if (hinv > 0) {\n\
            uv.y = (1.0 - uv.y);\n\
        }\n\
        vec4 s = texture2D(img, uv);\n\
        gl_FragColor = vec4(vec3(s.a), 1.0);\n\
    }\n\
}\n\
";

static const char *overlay_vert_src = "\
#version 100\n\
attribute vec2 a_vert;\n\
attribute vec2 a_texcoord;\n\
varying vec2 v_texcoord;\n\
uniform ivec2 dims;\n\
void main()\n\
{\n\
    v_texcoord = a_texcoord;\n\
    gl_Position = vec4(a_vert.x, a_vert.y, 0.0, 1.0);\n\
}\n\
";

static const char *overlay_frag_src = "\
#version 100\n\
precision mediump float;\n\
uniform ivec2 dims;\n\
uniform int hinv;\n\
uniform vec2 obj1;\n\
uniform int obj1_valid;\n\
uniform vec2 obj2;\n\
uniform int obj2_valid;\n\
uniform vec2 obj3;\n\
uniform int obj3_valid;\n\
varying vec2 v_texcoord;\n\
void main()\n\
{\n\
    float screen_hwratio = float(dims.y) / float(dims.x);\n\
    float video_whratio = 1.333333;\n\
    float u_max = screen_hwratio / video_whratio;\
    vec2 uv = v_texcoord.yx * vec2(u_max, 1.0);\
    float free_u = u_max - 1.0;\n\
    uv.x -= free_u / 2.0;\n\
    if (uv.x < 0.0 || uv.x > 1.0) {\
        gl_FragColor = vec4(vec3(0.0), 1.0);\n\
    } else {\n\
        if (hinv > 0) {\n\
            uv.y = (1.0 - uv.y);\n\
        }\n\
        vec3 color = vec3(0.0);\n\
        float a = 0.0;\n\
        if (obj1_valid > 0) {\n\
            float d = length((uv - obj1) * (uv - obj1));\n\
            if (d < 0.01 && d > 0.009) {\n\
                color.r = 1.0;\n\
                a = 0.5;\n\
            }\n\
        }\n\
        if (obj2_valid > 0) {\n\
            float d = length((uv - obj2) * (uv - obj2));\n\
            if (d < 0.5 && d > 0.45) {\n\
                color.g = 1.0;\n\
                a = 0.5;\n\
            }\n\
        }\n\
        if (obj3_valid > 0) {\n\
            float d = length((uv - obj3) * (uv - obj3));\n\
            if (d < 0.5 && d > 0.45) {\n\
                color.b = 1.0;\n\
                a = 0.5;\n\
            }\n\
        }\n\
        gl_FragColor = vec4(color, a);\n\
    }\n\
}\n\
";

class OpenGLVideoDisplay : public IVideoDisplay
{
public:
    OpenGLVideoDisplay(IWindow *window):
        _window(window),
        _img_mesh(_img_shader),
        _overlay_mesh(_overlay_shader),
        _next_frame(nullptr)
    {
        _overlay_shader.load(overlay_vert_src, overlay_frag_src);
        _overlay_shader.enable_attrib_vert("a_vert");
        _overlay_shader.enable_attrib_texcoord("a_texcoord");
        _overlay_shader.enable_uniform("dims");
        _overlay_shader.enable_uniform("hinv");
        _overlay_shader.enable_uniform("obj1");
        _overlay_shader.enable_uniform("obj2");
        _overlay_shader.enable_uniform("obj3");
        _overlay_shader.enable_uniform("obj1_valid");
        _overlay_shader.enable_uniform("obj2_valid");
        _overlay_shader.enable_uniform("obj3_valid");

        _img_shader.load(img_vert_src, img_frag_src);
        _img_shader.add_texture("img");
        _img_shader.enable_attrib_vert("a_vert");
        _img_shader.enable_attrib_texcoord("a_texcoord");
        _img_shader.enable_uniform("dims");
        _img_shader.enable_uniform("hinv");

        _img_mesh.make_rect(-1.0f, -1.0f, 2.0f, 2.0f);
        _overlay_mesh.make_rect(-1.0f, -1.0f, 2.0f, 2.0f);

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
    Shader _overlay_shader;
    Mesh _img_mesh;
    Mesh _overlay_mesh;
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

    struct obj_desc
    {
        std::string name;
        int valid;
        float x, y;
    };

    std::array<obj_desc, 3> objs;
    objs[0].name = "obj1";
    objs[0].valid = 0;
    objs[1].name = "obj2";
    objs[1].valid = 0;
    objs[2].name = "obj3";
    objs[2].valid = 0;

    objs[0].valid = 1;
    objs[0].x = 0.5f;
    objs[0].y = 0.0f;

    INFO("Window (%dx%d)\n", w, h);

    _window->poll();

    if (VideoFrame *frame = _next_frame.exchange(nullptr); frame) {
        auto &fmt = frame->format;
        _img_shader.update_texture("img", fmt.width, fmt.height, GL_ALPHA, GL_ALPHA, GL_UNSIGNED_BYTE, frame->buffer.data());

        INFO("Updated video frame!");

        delete frame;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    _img_shader.use();
    _img_shader.update_uniform("dims", std::array{w, h});
    _img_shader.update_uniform("hinv", 0);
    _img_mesh.draw();

    _overlay_shader.use();
    _overlay_shader.update_uniform("dims", std::array{w, h});
    _overlay_shader.update_uniform("hinv", 0);

    for (auto &obj : objs) {
        _overlay_shader.update_uniform(obj.name + "_valid", obj.valid);

        if (obj.valid) {
            _overlay_shader.update_uniform(obj.name,  std::array{obj.x, obj.y});
        }
    }

    _overlay_mesh.draw();

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
