#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "CppReflection/GetStaticTypeInfo.hpp"
#include "integer.hpp"
#include "opengl/gl_api.hpp"
#include "shader/define_handle.hpp"
#include "shader/uniform_handle.hpp"

class ShaderDefine;
class ShaderUniform;
class Texture;

class Shader
{
public:
    Shader(std::filesystem::path path);
    ~Shader();

    void Use();

    void Compile();
    [[nodiscard]] std::optional<ui32> FindUniformLocation(const char*) const noexcept;
    [[nodiscard]] ui32 GetUniformLocation(const char*) const noexcept;
    void DrawDetails();

    std::optional<UniformHandle> FindUniform(Name name) const noexcept;
    UniformHandle GetUniform(Name name) const;
    void SetUniform(UniformHandle& handle, edt::GUID type_guid, std::span<const ui8> data);

    void SetUniform(UniformHandle& handle, const std::shared_ptr<Texture>& texture);

    template <typename T>
    void SetUniform(UniformHandle& handle, const T& value)
    {
        SetUniform(
            handle,
            cppreflection::GetStaticTypeInfo<T>().guid,
            std::span<const ui8>(reinterpret_cast<const ui8*>(&value), sizeof(T)));
    }

    void SendUniforms();
    void SendUniform(UniformHandle&);

    template <typename T>
    const T& GetDefineValue(DefineHandle& handle) const;
    std::span<const ui8> GetDefineValue(DefineHandle& handle, edt::GUID type_guid) const;
    std::optional<DefineHandle> FindDefine(Name name) const noexcept;
    DefineHandle GetDefine(Name name) const;

    void SetDefineValue(DefineHandle& handle, edt::GUID type_guid, std::span<const ui8> value);

    template <typename T>
    void SetDefineValue(DefineHandle& handle, const T& value)
    {
        SetDefineValue(
            handle,
            cppreflection::GetStaticTypeInfo<T>().guid,
            std::span<const ui8>(reinterpret_cast<const ui8*>(&value), sizeof(T)));
    }

protected:
    ShaderUniform& GetUniform(UniformHandle& handle);
    const ShaderUniform& GetUniform(UniformHandle& handle) const;
    std::span<const ui8> GetUniformValueViewRaw(UniformHandle& handle, edt::GUID type_guid) const;
    void UpdateUniformHandle(UniformHandle& handle) const;
    void UpdateDefineHandle(DefineHandle& handle) const;

    template <typename T>
    const T& GetUniformValue(UniformHandle& handle)
    {
        std::span<const ui8> view = GetUniformValueViewRaw(handle, cppreflection::GetStaticTypeInfo<T>().guid);
        assert(view.size() == sizeof(T));
        return *reinterpret_cast<const T*>(view.data());
    }

private:
    void Check() const;
    void Destroy();
    void UpdateUniforms();

public:
    static std::filesystem::path shaders_dir_;

private:
    std::filesystem::path path_;
    std::vector<ShaderDefine> defines_;
    std::vector<ShaderUniform> uniforms_;
    std::optional<GLuint> program_;
    bool definitions_initialized_ : 1;
    bool need_recompile_ : 1;
};

template <typename T>
const T& Shader::GetDefineValue(DefineHandle& handle) const
{
    constexpr edt::GUID type_guid = cppreflection::GetStaticTypeInfo<T>().guid;
    auto value_view = GetDefineValue(handle, type_guid);
    return *reinterpret_cast<const T*>(value_view.data());
}
