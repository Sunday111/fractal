#pragma once

#include "klgl/opengl/gl_api.hpp"

namespace klgl
{

template <typename T>
struct TypeToGlType;

template <>
struct TypeToGlType<float>
{
    static constexpr size_t Size = 1;
    static constexpr GLenum Type = GL_FLOAT;
};

template <typename T, int N>
struct TypeToGlType<Eigen::Matrix<T, N, 1>>
{
    static constexpr size_t Size = static_cast<size_t>(N);
    static constexpr GLenum Type = TypeToGlType<T>::Type;
};

}  // namespace klgl
