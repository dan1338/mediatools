#include <GLES2/gl2.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>
#include <atomic>
#include "IVideoDisplay.h"
#include "log.h"

class IWindow
{
public:
    virtual auto resize(int width, int height) -> void = 0;
    virtual auto poll() -> void = 0;
    virtual auto should_close() -> bool = 0;
    virtual auto swap_buffers() -> void = 0;
};

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
#else
#include <EGL/egl.h>
#include <android/native_window.h>

class NullWindow : public IWindow
{
public:
    auto resize(int width, int height) -> void {}
    auto poll() -> void {}
    auto should_close() -> bool {return false;}
    auto swap_buffers() -> void {}
};

class AndroidWindow : public IWindow
{
public:
    AndroidWindow()
    {
    }

    void init_egl(ANativeWindow *awindow)
    {
        const EGLint attribs[] = {
                EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                EGL_BLUE_SIZE, 8,
                EGL_GREEN_SIZE, 8,
                EGL_RED_SIZE, 8,
                EGL_NONE
        };
        EGLint pi32ConfigAttribs[] = {
                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                EGL_RED_SIZE, 8,
                EGL_GREEN_SIZE, 8,
                EGL_BLUE_SIZE, 8,
                EGL_ALPHA_SIZE, 8,
                EGL_DEPTH_SIZE, 0,
                EGL_STENCIL_SIZE, 0,
                EGL_NONE
        };

        EGLDisplay display;
        EGLConfig config;
        EGLint numConfigs;
        EGLint format;
        EGLSurface surface;
        EGLContext context;
        EGLint width;
        EGLint height;
        GLfloat ratio;

        INFO("Initializing context");

        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        //display = eglGetCurrentDisplay();

        if (display == EGL_NO_DISPLAY) {
            ERROR("eglGetDisplay() returned error %d", eglGetError());
            return;
        }
        if (!eglInitialize(display, 0, 0)) {
            ERROR("eglInitialize() returned error %d", eglGetError());
            return;
        }

        eglBindAPI(EGL_OPENGL_ES_API);

        if (!eglChooseConfig(display, pi32ConfigAttribs, &config, 1, &numConfigs)) {
            ERROR("eglChooseConfig() returned error %d", eglGetError());
            //destroy();
            return;
        }

        if (!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format)) {
            ERROR("eglGetConfigAttrib() returned error %d", eglGetError());
            //destroy();
            return;
        }

        const EGLint ctx_attrs[] = {
                EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE
        };

        if (!(context = eglCreateContext(display, config, 0, ctx_attrs))) {
            ERROR("eglCreateContext() returned error %d", eglGetError());
            //destroy();
            return;
        }

        ANativeWindow_setBuffersGeometry(awindow, 0, 0, format);

        if (!(surface = eglCreateWindowSurface(display, config, awindow, 0))) {
            ERROR("eglCreateWindowSurface() returned error %d", eglGetError());
            //destroy();
            return;
        }

        if (!eglMakeCurrent(display, surface, surface, context)) {
            ERROR("eglMakeCurrent() returned error %d", eglGetError());
            //destroy();
            return;
        }

        if (!eglQuerySurface(display, surface, EGL_WIDTH, &width) ||
            !eglQuerySurface(display, surface, EGL_HEIGHT, &height)) {
            ERROR("eglQuerySurface() returned error %d", eglGetError());
            //destroy();
            return;
        }

        printf(
                "Vendor: %s, Renderer: %s, Version: %s\n",
                glGetString(GL_VENDOR),
                glGetString(GL_RENDERER),
                glGetString(GL_VERSION)
        ) ;

        printf("Extensions: %s\n", glGetString(GL_EXTENSIONS)) ;


        _display = display;
        _surface = surface;
        _context = context;

        //glDisable(GL_DITHER);
        //glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
        //glEnable(GL_CULL_FACE);
        //glShadeModel(GL_SMOOTH);
        //glEnable(GL_DEPTH_TEST);

        glViewport(0, 0, width, height);

        ratio = (GLfloat) width / height;
        //glMatrixMode(GL_PROJECTION);
        //glLoadIdentity();
        //glFrustumf(-ratio, ratio, -1, 1, 1, 10);
    }

    auto resize(int width, int height) -> void
    {
        // unsupported
    }

    auto poll() -> void
    {
        // unsupported
    }

    auto should_close() -> bool
    {
        return false;
    }

    auto swap_buffers() -> void
    {
        eglSwapBuffers(_display, _surface);
    }

private:
    EGLDisplay _display;
    EGLSurface _surface;
    EGLContext _context;
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

        if (GLenum err = glGetError(); err != GL_NO_ERROR) {
            while (err != GL_NO_ERROR) {
                ERROR("glGenTextures() -> -1 (%d)\n", err);
                err = glGetError();
            }
        }

        glActiveTexture(GL_TEXTURE0);
        tex_info.uniform_loc = glGetUniformLocation(_id, name.c_str());
    }

    void update_texture(const std::string &name, GLsizei w, GLsizei h, GLint intern_fmt, GLenum data_fmt, GLenum data_type, const void *data)
    {
        const auto &tex_info = _textures.at(name);

        INFO("glTexImage2D (%d w) (%d h) (%d ifmt) (%d dfmt) (%d dtype)\n", w, h, intern_fmt, data_fmt, data_type);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_info.id);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, intern_fmt, w, h, 0, data_fmt, data_type, data);

        if (GLenum err = glGetError(); err != GL_NO_ERROR) {
            while (err != GL_NO_ERROR) {
                ERROR("glTexImage() -> -1 (%d)\n", err);
                err = glGetError();
            }
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        if (GLenum err = glGetError(); err != GL_NO_ERROR) {
            while (err != GL_NO_ERROR) {
                ERROR("glGenerateMipmaps() -> -1 (%d)\n", err);
                err = glGetError();
            }
        }

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
    vec4 s = texture2D(img, v_texcoord);\n\
    gl_FragColor = vec4(vec3(s.a), 1.0);\n\
}\n\
";

class OpenGLVideoDisplay : public IVideoDisplay
{
public:
    OpenGLVideoDisplay(IWindow *window):
        _window(window),
        _img_mesh(_img_shader),
        _next_frame(nullptr)
    {
        _img_shader.load(img_vert_src, img_frag_src);
        _img_shader.add_texture("img");
        _img_shader.enable_attrib_vert("a_vert");
        _img_shader.enable_attrib_texcoord("a_texcoord");

        _img_mesh.make_rect(-1.0f, -1.0f, 2.0f, 2.0f);

        glClearColor(0.3, 0.0, 0.3, 1.0);
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
    _window->poll();

    if (VideoFrame *frame = _next_frame.exchange(nullptr); frame) {
        auto &fmt = frame->format;
        _img_shader.update_texture("img", fmt.width, fmt.height, GL_ALPHA, GL_ALPHA, GL_UNSIGNED_BYTE, frame->buffer.data());

        INFO("Updated video frame!");

        delete frame;
    }

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
#else
auto create_null_video_display() -> IVideoDisplay*
{
    return new OpenGLVideoDisplay(new NullWindow);
}

auto create_android_video_display(ANativeWindow *awindow) -> IVideoDisplay*
{
    auto *window = new AndroidWindow();
    window->init_egl(awindow);

    return new OpenGLVideoDisplay(window);
}
#endif
