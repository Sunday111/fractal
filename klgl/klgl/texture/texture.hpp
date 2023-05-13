#pragma once

#include <memory>
#include <optional>
#include <span>

#include "klgl/opengl/gl_api.hpp"
#include "klgl/wrap/wrap_eigen.hpp"

namespace klgl
{

class Texture
{
public:
    static std::unique_ptr<Texture> CreateEmpty(size_t width, size_t height, GLint internal_format = GL_RGB);

    ~Texture();

    void Bind() const;
    void SetPixels(std::span<const Eigen::Vector3<uint8_t>> pixel_data);
    Eigen::Vector2<size_t> GetSize() const
    {
        return {width_, height_};
    }
    size_t GetWidth() const
    {
        return width_;
    }
    size_t GetHeight() const
    {
        return height_;
    }
    std::optional<GLuint> GetTexture() const
    {
        return texture_;
    }

private:
    std::optional<GLuint> texture_;
    size_t width_;
    size_t height_;
    GLenum type_ = GL_TEXTURE_2D;
};

}  // namespace klgl
