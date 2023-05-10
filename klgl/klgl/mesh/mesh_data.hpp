#pragma once

#include <array>
#include <memory>
#include <vector>

#include "klgl/opengl/gl_api.hpp"
#include "klgl/wrap/wrap_eigen.hpp"

namespace klgl
{

struct MeshData
{
    static MeshData MakeIndexedQuad();

    std::vector<Eigen::Vector2f> vertices;
    std::vector<Eigen::Vector3i> indices;
};

struct MeshOpenGL
{
    static std::unique_ptr<MeshOpenGL> MakeFromData(const MeshData& mesh_data);

    void Bind() const;
    void Draw() const;

    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLuint topology = GL_TRIANGLES;
    size_t elements_count = 0;
};

}  // namespace klgl
