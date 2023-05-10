#include "mesh_data.hpp"

#include <algorithm>
#include <array>
#include <ranges>

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
    Bind();
    OpenGl::DrawElements(topology, elements_count, GL_UNSIGNED_INT, nullptr);
}

std::unique_ptr<MeshOpenGL> MeshOpenGL::MakeFromData(const MeshData& mesh_data)
{
    auto mesh = std::make_unique<MeshOpenGL>();

    mesh->vao = OpenGl::GenVertexArray();
    mesh->vbo = OpenGl::GenBuffer();
    mesh->ebo = OpenGl::GenBuffer();

    mesh->Bind();
    OpenGl::BindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
    OpenGl::BufferData(GL_ARRAY_BUFFER, std::span{mesh_data.vertices}, GL_STATIC_DRAW);
    OpenGl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
    OpenGl::BufferData(GL_ELEMENT_ARRAY_BUFFER, std::span{mesh_data.indices}, GL_STATIC_DRAW);

    mesh->elements_count = mesh_data.indices.size() * 3;

    return mesh;
}
}  // namespace klgl
