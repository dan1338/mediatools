#include "IVideoDisplay.h"
#include <GLES2/gl2.h>
#ifndef __ANDROID__
#include <GLFW/glfw3.h>
#endif
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

class IWindow
{
public:
    virtual auto resize(int width, int height) -> void = 0;
    virtual auto poll() -> void = 0;
    virtual auto should_close() -> bool = 0;
    virtual auto swap_buffers() -> void = 0;
};

#ifndef __ANDROID__
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

class Mesh
{
public:
    Mesh()
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

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, (void*)(sizeof(float)*2));

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
    unsigned int _vbo, _ebo;

    std::vector<float> _verts;
    std::vector<unsigned short> _indices;
};

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

    void add_texture(const std::string &name)
    {
        TexInfo &tex_info = _textures[name];

        glGenTextures(1, &tex_info.id);
        tex_info.uniform_loc = glGetUniformLocation(_id, name.c_str());
    }

    void update_texture(const std::string &name, GLsizei w, GLsizei h, GLint intern_fmt, GLenum data_fmt, GLenum data_type, const void *data)
    {
        const auto &tex_info = _textures.at(name);

        glBindTexture(GL_TEXTURE_2D, tex_info.id);
        glTexImage2D(GL_TEXTURE_2D, 0, intern_fmt, w, h, 0, data_fmt, data_type, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    void use() const
    {
        int tex_slot = 0;

        for (const auto &[tex_name, tex_info] : _textures)
        {
            glActiveTexture(GL_TEXTURE0 + tex_slot);
            glBindTexture(GL_TEXTURE_2D, tex_info.id);
            glUniform1i(tex_info.uniform_loc, tex_slot);

            ++tex_slot;
        }

        glUseProgram(_id);
    }

private:
    int _id;

    struct TexInfo
    {
        GLuint id;
        GLint uniform_loc;
    };

    std::unordered_map<std::string, TexInfo> _textures;

    void check_shader_status(const char *name, int shader)
    {
        int ok;
        char err[256];

        glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);

        if (!ok)
        {
            glGetShaderInfoLog(shader, sizeof err, 0, err);
            printf("%s error: %s\n", name, err);
            exit(1);
        }
    }
};

static const char *img_vert_src = "\
#version 100\n\
attribute vec2 a_vert;\n\
attribute vec2 a_texcoord;\n\
varying vec2 v_texcoord;\n\
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
varying vec2 v_texcoord;\n\
void main()\n\
{\n\
    gl_FragColor = texture2D(img, v_texcoord);\n\
}\n\
";

class OpenGLVideoDisplay : public IVideoDisplay
{
public:
    OpenGLVideoDisplay(IWindow *window): _window(window)
    {
        _img_mesh.make_rect(-1.0f, -1.0f, 2.0f, 2.0f);
        _img_shader.load(img_vert_src, img_frag_src);
        _img_shader.add_texture("img");

        glClearColor(0.0, 0.0, 0.3, 1.0);
    }

    virtual auto open() -> void override {}
    virtual auto set_video_frame(const VideoFramePtr&) -> void override;
    virtual auto update() -> bool override;

private:
    IWindow *_window;

    Shader _img_shader;
    Mesh _img_mesh;
};

auto OpenGLVideoDisplay::set_video_frame(const VideoFramePtr& frame) -> void
{
    const auto &fmt = frame->format;

    _img_shader.update_texture("img", fmt.width, fmt.height, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, frame->buffer.data());
}

auto OpenGLVideoDisplay::update() -> bool
{
    _window->poll();

    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    _img_shader.use();
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
