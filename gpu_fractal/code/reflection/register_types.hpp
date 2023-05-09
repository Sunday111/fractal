#pragma once

#include "CppReflection/GetTypeInfo.hpp"
#include "CppReflection/TypeRegistry.hpp"
#include "EverydayTools/GUID_fmtlib.hpp"
#include "reflection/eigen_reflect.hpp"
#include "shader/sampler_uniform.hpp"

inline void RegisterReflectionTypes()
{
    [[maybe_unused]] const cppreflection::Type* t;
    t = cppreflection::GetTypeInfo<float>();
    t = cppreflection::GetTypeInfo<int8_t>();
    t = cppreflection::GetTypeInfo<int16_t>();
    t = cppreflection::GetTypeInfo<int32_t>();
    t = cppreflection::GetTypeInfo<int64_t>();
    t = cppreflection::GetTypeInfo<uint8_t>();
    t = cppreflection::GetTypeInfo<uint16_t>();
    t = cppreflection::GetTypeInfo<uint32_t>();
    t = cppreflection::GetTypeInfo<uint64_t>();
    t = cppreflection::GetTypeInfo<Eigen::Vector3f>();
    t = cppreflection::GetTypeInfo<Eigen::Vector4f>();
    t = cppreflection::GetTypeInfo<Eigen::Matrix3f>();
    t = cppreflection::GetTypeInfo<Eigen::Matrix4f>();
    t = cppreflection::GetTypeInfo<Eigen::Vector2f>();
    t = cppreflection::GetTypeInfo<Eigen::Vector3f>();
    t = cppreflection::GetTypeInfo<Eigen::Vector4f>();
    t = cppreflection::GetTypeInfo<Eigen::Matrix3f>();
    t = cppreflection::GetTypeInfo<Eigen::Matrix4f>();
    t = cppreflection::GetTypeInfo<SamplerUniform>();
}