#pragma once

#include "klgl/opengl/gl_api.hpp"
#include "klgl/template/class_member_traits.hpp"
#include "klgl/template/member_offset.hpp"
#include "klgl/template/type_to_gl_type.hpp"
#include "klgl/wrap/wrap_eigen.hpp"


struct MeshVertex
{
    Eigen::Vector2f position;
    Eigen::Vector2f tex_coord;
};

template <auto MemberVariablePtr>
void RegisterAttribute(GLuint location, bool normalized)
{
    using MemberTraits = ClassMemberTraits<decltype(MemberVariablePtr)>;
    using GlTypeTraits = TypeToGlType<typename MemberTraits::Member>;
    const size_t vertex_stride = sizeof(typename MemberTraits::Class);
    const size_t member_stride = MemberOffset<MemberVariablePtr>();
    OpenGl::VertexAttribPointer(
        location,
        GlTypeTraits::Size,
        GlTypeTraits::Type,
        normalized,
        vertex_stride,
        reinterpret_cast<void*>(member_stride));
    OpenGl::EnableVertexAttribArray(location);
}
