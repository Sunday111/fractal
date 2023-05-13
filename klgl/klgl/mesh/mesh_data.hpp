#pragma once

#include <array>
#include <memory>
#include <span>
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
    template <typename Vertex, size_t VE = std::dynamic_extent, size_t IE = std::dynamic_extent>
    static std::unique_ptr<MeshOpenGL> MakeFromData(
        std::span<const Vertex, VE> vertices,
        std::span<const uint32_t, IE> indices)
    {
        auto mesh = std::make_unique<MeshOpenGL>();

        mesh->vao = OpenGl::GenVertexArray();
        mesh->vbo = OpenGl::GenBuffer();
        mesh->ebo = OpenGl::GenBuffer();

        mesh->Bind();
        OpenGl::BindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
        OpenGl::BufferData(GL_ARRAY_BUFFER, vertices, GL_STATIC_DRAW);
        OpenGl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
        OpenGl::BufferData(GL_ELEMENT_ARRAY_BUFFER, std::span{indices}, GL_STATIC_DRAW);

        mesh->elements_count = indices.size();
        mesh->topology = GL_TRIANGLES;

        return mesh;
    }

    ~MeshOpenGL();

    void Bind() const;
    void BindAndDraw() const;
    void Draw() const;

    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLuint topology = GL_TRIANGLES;
    size_t elements_count = 0;
};

}  // namespace klgl
