#pragma once

#include "CppReflection/ReflectionProvider.hpp"
#include "CppReflection/StaticType/class.hpp"
#include "wrap/wrap_eigen.hpp"

namespace cppreflection {

template <>
struct TypeReflectionProvider<Eigen::Vector2f> {
  [[nodiscard]] inline constexpr static auto ReflectType() {
    return cppreflection::StaticClassTypeInfo<Eigen::Vector2f>(
        "Eigen::Vector2f",
        edt::GUID::Create("C3B7C262-17D2-403F-B412-9A5A566EC85E"));
  }
};

template <>
struct TypeReflectionProvider<Eigen::Vector3f> {
  [[nodiscard]] inline constexpr static auto ReflectType() {
    return cppreflection::StaticClassTypeInfo<Eigen::Vector3f>(
        "Eigen::Vector3f",
        edt::GUID::Create("B3A1359C-8E14-4FA6-8EDD-F6885062E548"));
  }
};

template <>
struct TypeReflectionProvider<Eigen::Vector4f> {
  [[nodiscard]] inline constexpr static auto ReflectType() {
    return cppreflection::StaticClassTypeInfo<Eigen::Vector4f>(
        "Eigen::Vector4f",
        edt::GUID::Create("6979B02A-B644-4D25-9DEF-4268DC031EAF"));
  }
};

template <>
struct TypeReflectionProvider<Eigen::Matrix3f> {
  [[nodiscard]] inline constexpr static auto ReflectType() {
    return cppreflection::StaticClassTypeInfo<Eigen::Matrix3f>(
        "Eigen::Matrix3f",
        edt::GUID::Create("F1E4DE8F-72B4-47E1-8FB2-A482AA094731"));
  }
};

template <>
struct TypeReflectionProvider<Eigen::Matrix4f> {
  [[nodiscard]] inline constexpr static auto ReflectType() {
    return cppreflection::StaticClassTypeInfo<Eigen::Matrix4f>(
        "Eigen::Matrix4f",
        edt::GUID::Create("F9078916-1820-474F-AE36-60BBDB1BA52B"));
  }
};

}  // namespace cppreflection