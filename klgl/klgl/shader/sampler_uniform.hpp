#pragma once

#include <memory>

#include "CppReflection/GetStaticTypeInfo.hpp"

namespace klgl
{

class Texture
{
};

class SamplerUniform
{
public:
    uint8_t sampler_index;
    std::shared_ptr<Texture> texture;
};

}  // namespace klgl

namespace cppreflection
{
template <>
struct TypeReflectionProvider<::klgl::SamplerUniform>
{
    [[nodiscard]] inline constexpr static auto ReflectType()
    {
        return StaticClassTypeInfo<::klgl::SamplerUniform>(
            "SamplerUniform",
            edt::GUID::Create("2FBEEB94-BBB3-491C-A299-AD1960641D3F"));
    }
};
}  // namespace cppreflection
