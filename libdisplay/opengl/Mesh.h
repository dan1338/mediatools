#pragma once

#include <GLES2/gl2.h>
#include "Shader.h"

class Mesh
{
public:
    Mesh(const Shader &shader): _shader(shader)
    {
        glGenBuffers(1, &_vbo);
        glGenBuffers(1, &_ebo);
    }

    ~Mesh()
    {
        glDeleteBuffers(1, &_vbo);
        glDeleteBuffers(1, &_ebo);
    }

    void make_rect(float x, float y, float w, float h)
    {
         _shader.use();

        _vertices.resize(4);
        _vertices[0] = {{x, y}, {0.0f, 1.0f}, 0};
        _vertices[1] = {{x + w, y}, {1.0f, 1.0f}, 1};
        _vertices[2] = {{x + w, y + h}, {1.0f, 0.0f}, 2};
        _vertices[3] = {{x, y + h}, {0.0f, 0.0f}, 3};

        glBindBuffer(GL_ARRAY_BUFFER, _vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(VertexAttribute)*4, _vertices.data(), GL_STATIC_DRAW);

        _indices = {0, 3, 2, 2, 1, 0};

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short)*6, _indices.data(), GL_STATIC_DRAW);
    }

    void draw() const
    {
        _shader.use();

        if (auto pos_loc = _shader.get_attrib_loc("pos"); pos_loc.has_value()) {
            glEnableVertexAttribArray(*pos_loc);
            glVertexAttribPointer(*pos_loc, 2, GL_FLOAT, GL_FALSE, sizeof(VertexAttribute), (void*)0);
        }
        if (auto uv_loc = _shader.get_attrib_loc("uv"); uv_loc.has_value()) {
            glEnableVertexAttribArray(*uv_loc);
            glVertexAttribPointer(*uv_loc, 2, GL_FLOAT, GL_FALSE, sizeof(VertexAttribute), (void*)(sizeof(float)*2));
        }
        if (auto idx_loc = _shader.get_attrib_loc("idx"); idx_loc.has_value()) {
            glEnableVertexAttribArray(*idx_loc);
            glVertexAttribPointer(*idx_loc, 1, GL_FLOAT, GL_FALSE, sizeof(VertexAttribute), (void*)(sizeof(float)*4));
        }

        glBindBuffer(GL_ARRAY_BUFFER, _vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
    }

private:
    const Shader &_shader;
    unsigned int _vbo, _ebo;

    struct VertexAttribute
    {
        float pos[2];
        float uv[2];
        float index;
    };

    std::vector<VertexAttribute> _vertices;
    std::vector<unsigned short> _indices;
};

