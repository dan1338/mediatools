#pragma once

#include <GLES2/gl2.h>
#include <optional>
#include <unordered_map>
#include <string>

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

        glUseProgram(_id);
    }

    void enable_attrib(const std::string &name)
    {
        _attribs[name] = glGetAttribLocation(_id, name.c_str());
    }

    void enable_uniform(const std::string &name)
    {
        _uniforms[name] = glGetUniformLocation(_id, name.c_str());
    }

    void configure(const std::vector<std::string> &attribs, const std::vector<std::string> &uniforms)
    {
        for (const auto &name : attribs)
            enable_attrib(name);
        for (const auto &name : uniforms)
            enable_uniform(name);
    }

    std::optional<GLint> get_attrib_loc(const std::string &name) const
    {
        if (auto it = _attribs.find(name); it != _attribs.end()) {
            return it->second;
        }

        return {};
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

    struct TexInfo
    {
        GLuint id;
        GLint uniform_loc;
    };

    std::unordered_map<std::string, TexInfo> _textures;
    std::unordered_map<std::string, GLint> _uniforms;
    std::unordered_map<std::string, GLint> _attribs;

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

