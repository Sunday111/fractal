#pragma once

#include "EverydayTools/Math/constants.hpp"
#include "klgl/macro/warning_suppress.hpp"

// clang-format off
#ifdef __clang__
    #define include_eigen_begin \
        warning_push \
        _Pragma("GCC diagnostic ignored \"-Wsign-conversion\"") \
        _Pragma("GCC diagnostic ignored \"-Wzero-as-null-pointer-constant\"") \
        _Pragma("GCC diagnostic ignored \"-Wold-style-cast\"") \
        _Pragma("GCC diagnostic ignored \"-Wfloat-equal\"") \
        static_assert(true, "")
#elif defined(__GNUC__) || defined(__GNUG__)
    #define include_eigen_begin \
        warning_push \
        _Pragma("GCC diagnostic ignored \"-Wsign-conversion\"") \
        _Pragma("GCC diagnostic ignored \"-Wzero-as-null-pointer-constant\"") \
        _Pragma("GCC diagnostic ignored \"-Wold-style-cast\"") \
        static_assert(true, "")
#elif defined(_MSC_VER)
    #define include_eigen_begin \
        warning_push \
        _Pragma("warning(disable : 4201)") \
        _Pragma("warning(disable : 5054)") \
        static_assert(true, "")
#endif

#define include_eigen_end warning_pop static_assert(true, "")

// clang-format on

include_eigen_begin;
#include "Eigen/Dense"
include_eigen_end;

#include <cmath>
#include <concepts>

namespace Eigen
{

/// @brief Returns a perspective transformation matrix like the one from
/// gluPerspective
/// @see http://www.opengl.org/sdk/docs/man2/xhtml/gluPerspective.xml
/// @see glm::perspective
template <std::floating_point Scalar>
Eigen::Matrix<Scalar, 4, 4> Perspective(Scalar fovy, Scalar aspect, Scalar zNear, Scalar zFar)
{
    Transform<Scalar, 3, Projective> tr;
    tr.matrix().setZero();
    assert(aspect > 0);
    assert(zFar > zNear);
    assert(zNear > 0);
    Scalar radf = edt::PI<Scalar> * fovy / 180;
    Scalar tan_half_fovy = std::tan(radf / 2);
    tr(0, 0) = Scalar(1) / (aspect * tan_half_fovy);
    tr(1, 1) = Scalar(1) / (tan_half_fovy);
    tr(2, 2) = -(zFar + zNear) / (zFar - zNear);
    tr(3, 2) = -Scalar(1);
    tr(2, 3) = -(2 * zFar * zNear) / (zFar - zNear);
    return tr.matrix();
}

template <typename Scalar>
Eigen::Matrix<Scalar, 4, 4> Scale(Scalar x, Scalar y, Scalar z)
{
    Transform<Scalar, 3, Affine> tr;
    tr.matrix().setZero();
    tr(0, 0) = x;
    tr(1, 1) = y;
    tr(2, 2) = z;
    tr(3, 3) = 1;
    return tr.matrix();
}

template <typename Scalar>
Eigen::Matrix<Scalar, 4, 4> Translate(Scalar x, Scalar y, Scalar z)
{
    Transform<Scalar, 3, Affine> tr;
    tr.matrix().setIdentity();
    tr(0, 3) = x;
    tr(1, 3) = y;
    tr(2, 3) = z;
    return tr.matrix();
}

/// @brief Returns a view transformation matrix like the one from glu's lookAt
/// @see http://www.opengl.org/sdk/docs/man2/xhtml/gluLookAt.xml
/// @see glm::lookAt
template <typename Derived>
Eigen::Matrix<typename Derived::Scalar, 4, 4> LookAt(const Derived& eye, const Derived& center, const Derived& up)
{
    typedef Eigen::Matrix<typename Derived::Scalar, 4, 4> Matrix4;
    typedef Eigen::Matrix<typename Derived::Scalar, 3, 1> Vector3;
    Vector3 f = (center - eye).normalized();
    Vector3 u = up.normalized();
    Vector3 s = f.cross(u).normalized();
    u = s.cross(f);
    Matrix4 mat = Matrix4::Zero();
    mat(0, 0) = s.x();
    mat(0, 1) = s.y();
    mat(0, 2) = s.z();
    mat(0, 3) = -s.dot(eye);
    mat(1, 0) = u.x();
    mat(1, 1) = u.y();
    mat(1, 2) = u.z();
    mat(1, 3) = -u.dot(eye);
    mat(2, 0) = -f.x();
    mat(2, 1) = -f.y();
    mat(2, 2) = -f.z();
    mat(2, 3) = f.dot(eye);
    mat.row(3) << 0, 0, 0, 1;
    return mat;
}

/// @see glm::ortho
template <typename Scalar>
Eigen::Matrix<Scalar, 4, 4> Ortho(
    Scalar const& left,
    Scalar const& right,
    Scalar const& bottom,
    Scalar const& top,
    Scalar const& zNear,
    Scalar const& zFar)
{
    Eigen::Matrix<Scalar, 4, 4> mat = Eigen::Matrix<Scalar, 4, 4>::Identity();
    mat(0, 0) = Scalar(2) / (right - left);
    mat(1, 1) = Scalar(2) / (top - bottom);
    mat(2, 2) = -Scalar(2) / (zFar - zNear);
    mat(3, 0) = -(right + left) / (right - left);
    mat(3, 1) = -(top + bottom) / (top - bottom);
    mat(3, 2) = -(zFar + zNear) / (zFar - zNear);
    return mat;
}

}  // namespace Eigen