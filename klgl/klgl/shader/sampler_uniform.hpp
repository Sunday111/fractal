#pragma once

#include <memory>

#include "CppReflection/GetStaticTypeInfo.hpp"
#include "klgl/integer.hpp"

class Texture
{
};

class SamplerUniform
{
public:
    ui8 sampler_index;
    std::shared_ptr<Texture> texture;
};

namespace cppreflection
{
template <>
struct TypeReflectionProvider<SamplerUniform>
{
    [[nodiscard]] inline constexpr static auto ReflectType()
    {
        return StaticClassTypeInfo<SamplerUniform>(
            "SamplerUniform",
            edt::GUID::Create("2FBEEB94-BBB3-491C-A299-AD1960641D3F"));
    }
};
}  // namespace cppreflection