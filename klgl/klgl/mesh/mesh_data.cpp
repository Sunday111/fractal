#include "mesh_data.hpp"

#include <algorithm>
#include <array>
#include <ranges>

#include "klgl/opengl/debug/annotations.hpp"

namespace klgl
{
MeshData MeshData::MakeIndexedQuad()
{
    const std::array<Eigen::Vector2f, 4> qvertices{{{1.0f, 1.0f}, {1.0f, -1.0f}, {-1.0f, -1.0f}, {-1.0f, 1.0f}}};
    const std::array<Eigen::Vector3i, 2> qindices{{{0, 1, 3}, {1, 2, 3}}};

    MeshData data{};
    data.vertices.resize(qvertices.size());
    data.indices.resize(qindices.size());

    std::ranges::copy(qvertices, data.vertices.begin());
    std::ranges::copy(qindices, data.indices.begin());

    return data;
}

void MeshOpenGL::Bind() const
{
    OpenGl::BindVertexArray(vao);
}

void MeshOpenGL::Draw() const
{
    assert(topology == GL_TRIANGLES);
    OpenGl::DrawElements(topology, elements_count, GL_UNSIGNED_INT, nullptr);
}

void MeshOpenGL::BindAndDraw() const
{
    Bind();
    Draw();
}

MeshOpenGL::~MeshOpenGL()
{
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);
}
}  // namespace klgl
