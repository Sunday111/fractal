#include "klgl/shader/shader.hpp"

#include <array>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <vector>

#include "CppReflection/TypeRegistry.hpp"
#include "fmt/format.h"
#include "klgl/read_file.hpp"
#include "klgl/reflection/eigen_reflect.hpp"
#include "klgl/shader/sampler_uniform.hpp"
#include "klgl/shader/shader.hpp"
#include "klgl/shader/shader_define.hpp"
#include "klgl/shader/shader_uniform.hpp"
#include "klgl/template/on_scope_leave.hpp"
#include "klgl/type_id_widget.hpp"
#include "klgl/wrap/wrap_imgui.hpp"
#include "nlohmann/json.hpp"

namespace klgl
{

std::filesystem::path Shader::shaders_dir_;

static void
CompileShader(GLuint shader, const std::filesystem::path& path, const std::vector<std::string>& extra_sources)
{
    constexpr size_t stack_reserved = 30;

    fmt::print("compiling shader {}\n", path.stem().string());
    std::vector<char> buffer;
    ReadFile(path, buffer);

    std::vector<const char*> shader_sources_heap;
    std::vector<GLint> shader_sources_lengths_heap;
    const char* shader_sources_stack[stack_reserved]{};
    GLint shader_sources_lengths_stack[stack_reserved]{};
    const size_t num_sources = extra_sources.size() + 1u;
    const char** shader_sources;
    GLint* shader_sources_lengths;
    if (num_sources > stack_reserved)
    {
        shader_sources_heap.resize(num_sources);
        shader_sources_lengths_heap.resize(num_sources);
        shader_sources = shader_sources_heap.data();
        shader_sources_lengths = shader_sources_lengths_heap.data();
    }
    else
    {
        shader_sources = shader_sources_stack;
        shader_sources_lengths = shader_sources_lengths_stack;
    }

    for (size_t i = 0; i < extra_sources.size(); ++i)
    {
        shader_sources[i] = extra_sources[i].data();
        shader_sources_lengths[i] = static_cast<GLsizei>(extra_sources[i].size());
    }

    shader_sources[extra_sources.size()] = buffer.data();
    shader_sources_lengths[extra_sources.size()] = static_cast<GLsizei>(buffer.size());

    glShaderSource(shader, static_cast<GLsizei>(num_sources), shader_sources, shader_sources_lengths);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    [[unlikely]] if (!success)
    {
        GLint info_length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_length);
        std::string error_info;
        if (info_length > 0)
        {
            error_info.resize(static_cast<size_t>(info_length));
            glGetShaderInfoLog(shader, info_length, nullptr, error_info.data());
        }
        throw std::runtime_error(fmt::format("failed to compile shader {} log:\n{}", path.stem().string(), error_info));
    }
}

static GLuint LinkShaders(const std::span<const GLuint>& shaders)
{
    GLuint program = glCreateProgram();
    for (auto shader : shaders)
    {
        glAttachShader(program, shader);
    }

    glLinkProgram(program);
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    [[likely]] if (success)
    {
        return program;
    }

    GLint info_length;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_length);
    std::string error_info;
    if (info_length > 0)
    {
        error_info.resize(static_cast<size_t>(info_length));
        glGetShaderInfoLog(program, info_length, nullptr, error_info.data());
    }
    glDeleteProgram(program);
    throw std::runtime_error(fmt::format("failed to link shaders. Log:\n{}", error_info));
}

static auto get_shader_json(const std::filesystem::path& path)
{
    std::ifstream shader_file(path);

    [[unlikely]] if (!shader_file.is_open())
    {
        throw std::invalid_argument(fmt::format("Failed to open file {}", path.string()));
    }

    nlohmann::json shader_json;
    shader_file >> shader_json;
    return shader_json;
}

Shader::Shader(std::filesystem::path path) : path_(std::move(path))
{
    definitions_initialized_ = false;
    need_recompile_ = false;
    Compile();
}

Shader::~Shader()
{
    Destroy();
}
void Shader::Use()
{
    Check();
    OpenGl::UseProgram(*program_);
}

std::optional<uint32_t> Shader::FindUniformLocation(const char* name) const noexcept
{
    Check();
    return OpenGl::FindUniformLocation(*program_, name);
}

uint32_t Shader::GetUniformLocation(const char* name) const noexcept
{
    Check();
    return OpenGl::GetUniformLocation(*program_, name);
}

void Shader::Compile()
{
    Destroy();

    nlohmann::json shader_json = get_shader_json(shaders_dir_ / path_);

    size_t num_compiled = 0;
    std::array<GLuint, 2> compiled;
    auto deleter = OnScopeLeave(
        [&]()
        {
            for (size_t i = 0; i < num_compiled; ++i)
            {
                glDeleteShader(compiled[i]);
            }
        });

    std::vector<std::string> extra_sources;

    {
        const std::string version = shader_json.at("glsl_version");
        std::string line = fmt::format("#version {}\n\n", version);
        extra_sources.push_back(line);
    }

    if (!definitions_initialized_)
    {
        if (shader_json.contains("definitions"))
        {
            for (const auto& def_json : shader_json["definitions"])
            {
                defines_.push_back(ShaderDefine::ReadFromJson(def_json));
            }
        }

        definitions_initialized_ = true;
    }

    for (const auto& definition : defines_)
    {
        extra_sources.push_back(definition.GenDefine());
    }

    auto add_one = [&](GLuint type, const char* json_name)
    {
        if (!shader_json.contains(json_name))
        {
            return;
        }

        auto stage_path = shaders_dir_ / "src" / std::string(shader_json[json_name]);
        const GLuint shader = glCreateShader(type);

        try
        {
            CompileShader(shader, stage_path, extra_sources);
        }
        catch (...)
        {
            glDeleteShader(shader);
            throw;
        }

        compiled[num_compiled] = shader;
        num_compiled += 1;
    };

    add_one(GL_VERTEX_SHADER, "vertex");
    add_one(GL_FRAGMENT_SHADER, "fragment");

    program_ = LinkShaders(std::span(compiled).subspan(0, num_compiled));
    need_recompile_ = false;

    UpdateUniforms();
}

void Shader::DrawDetails()
{
    if (ImGui::TreeNode("Static Variables"))
    {
        for (ShaderDefine& definition : defines_)
        {
            bool value_changed = false;
            SimpleTypeWidget(definition.type_guid, definition.name.GetView(), definition.value.data(), value_changed);

            if (value_changed)
            {
                need_recompile_ = true;
            }
        }
        ImGui::TreePop();
    }

    constexpr size_t stack_val_bytes = 64;
    uint64_t stack_val_arr[stack_val_bytes / 8];

    if (ImGui::TreeNode("Dynamic Variables"))
    {
        for (ShaderUniform& uniform : uniforms_)
        {
            auto type_info = cppreflection::GetTypeRegistry()->FindType(uniform.GetTypeGUID());

            type_info->GetSpecialMembers().copyConstructor(
                reinterpret_cast<void*>(stack_val_arr),
                uniform.GetValue().data());
            assert(stack_val_bytes >= type_info->GetInstanceSize());

            std::span<uint8_t> val_view(reinterpret_cast<uint8_t*>(stack_val_arr), type_info->GetInstanceSize());

            bool value_changed = false;
            SimpleTypeWidget(uniform.GetTypeGUID(), uniform.GetName().GetView(), val_view.data(), value_changed);

            if (value_changed)
            {
                uniform.SetValue(val_view);
            }

            type_info->GetSpecialMembers().destructor(val_view.data());
        }
        ImGui::TreePop();
    }

    if (need_recompile_)
    {
        Compile();
    }
}

std::span<const uint8_t> Shader::GetDefineValue(DefineHandle& handle, edt::GUID type_guid) const
{
    UpdateDefineHandle(handle);

    auto& define = defines_[handle.index];
    [[unlikely]] if (define.type_guid != type_guid)
    {
        auto type_registry = cppreflection::GetTypeRegistry();
        throw std::runtime_error(fmt::format(
            "Trying to interpret define {} of type {} as {}",
            define.name.GetView(),
            type_registry->FindType(define.type_guid)->GetName(),
            type_registry->FindType(type_guid)->GetName()));
    }

    return std::span(define.value.begin(), define.value.size());
}

void Shader::SetDefineValue(DefineHandle& handle, edt::GUID type_guid, std::span<const uint8_t> value)
{
    UpdateDefineHandle(handle);
    ShaderDefine& define = defines_[handle.index];
    define.SetValue(value);
    need_recompile_ = true;

    [[unlikely]] if (define.type_guid != type_guid)
        throw std::runtime_error(fmt::format("wrong type"));
}

std::optional<DefineHandle> Shader::FindDefine(Name name) const noexcept
{
    std::optional<DefineHandle> result;
    for (size_t index = 0; index < defines_.size(); ++index)
    {
        const ShaderDefine& define = defines_[index];
        if (define.name == name)
        {
            DefineHandle h;
            h.name = name;
            h.index = static_cast<uint32_t>(index);
            result = h;
            break;
        }
    }

    return result;
}

DefineHandle Shader::GetDefine(Name name) const
{
    [[likely]] if (auto maybe_handle = FindDefine(name); maybe_handle)
    {
        return *maybe_handle;
    }

    throw std::runtime_error(fmt::format("Define is not found: \"{}\"", name.GetView()));
}

std::optional<UniformHandle> Shader::FindUniform(Name name) const noexcept
{
    std::optional<UniformHandle> result;
    for (size_t index = 0; index < uniforms_.size(); ++index)
    {
        const ShaderUniform& uniform = uniforms_[index];
        if (uniform.GetName() == name)
        {
            UniformHandle h;
            h.name = name;
            h.index = static_cast<uint32_t>(index);
            result = h;
            break;
        }
    }

    return result;
}

UniformHandle Shader::GetUniform(Name name) const
{
    [[likely]] if (auto maybe_handle = FindUniform(name); maybe_handle)
    {
        return *maybe_handle;
    }

    throw std::runtime_error(fmt::format("Uniform is not found: \"{}\"", name.GetView()));
}

ShaderUniform& Shader::GetUniform(UniformHandle& handle)
{
    UpdateUniformHandle(handle);
    return uniforms_[handle.index];
}

const ShaderUniform& Shader::GetUniform(UniformHandle& handle) const
{
    UpdateUniformHandle(handle);
    return uniforms_[handle.index];
}

std::span<const uint8_t> Shader::GetUniformValueViewRaw(UniformHandle& handle, edt::GUID type_guid) const
{
    auto& uniform = GetUniform(handle);
    uniform.EnsureTypeMatch(type_guid);
    return uniform.GetValue();
}

void Shader::UpdateUniformHandle(UniformHandle& handle) const
{
    if (handle.index >= uniforms_.size() || uniforms_[handle.index].GetName() != handle.name)
    {
        handle = GetUniform(handle.name);
    }
}

void Shader::UpdateDefineHandle(DefineHandle& handle) const
{
    [[unlikely]] if (handle.index >= defines_.size() || defines_[handle.index].name != handle.name)
    {
        handle = GetDefine(handle.name);
    }
}

void Shader::SetUniform(UniformHandle& handle, edt::GUID type_guid, std::span<const uint8_t> value)
{
    auto& uniform = GetUniform(handle);
    uniform.EnsureTypeMatch(type_guid);
    uniform.SetValue(value);
}

void Shader::SetUniform(UniformHandle& handle, const std::shared_ptr<Texture>& texture)
{
    auto sampler_uniform = GetUniformValue<SamplerUniform>(handle);
    ShaderUniform& u = uniforms_[handle.index];
    sampler_uniform.texture = texture;
    u.SetValue(std::span(reinterpret_cast<const uint8_t*>(&sampler_uniform), sizeof(sampler_uniform)));
}

void Shader::SendUniforms()
{
    for (const ShaderUniform& uniform : uniforms_)
    {
        uniform.SendValue();
    }
}

void Shader::SendUniform(UniformHandle& handle)
{
    ShaderUniform& uniform = GetUniform(handle);
    uniform.SendValue();
}

void Shader::Check() const
{
    [[unlikely]] if (!program_)
    {
        throw std::runtime_error("Invalid shader");
    }
}

void Shader::Destroy()
{
    if (program_)
    {
        glDeleteProgram(*program_);
        program_.reset();
    }
}

static std::optional<edt::GUID> ConvertGlType(GLenum gl_type)
{
    switch (gl_type)
    {
    case GL_FLOAT:
        return cppreflection::GetStaticTypeInfo<float>().guid;
        break;

    case GL_FLOAT_VEC2:
        return cppreflection::GetStaticTypeInfo<Eigen::Vector2f>().guid;
        break;

    case GL_FLOAT_VEC3:
        return cppreflection::GetStaticTypeInfo<Eigen::Vector3f>().guid;
        break;

    case GL_FLOAT_VEC4:
        return cppreflection::GetStaticTypeInfo<Eigen::Vector4f>().guid;
        break;

    case GL_FLOAT_MAT3:
        return cppreflection::GetStaticTypeInfo<Eigen::Matrix3f>().guid;
        break;

    case GL_FLOAT_MAT4:
        return cppreflection::GetStaticTypeInfo<Eigen::Matrix4f>().guid;
        break;

    case GL_SAMPLER_2D:
        return cppreflection::GetStaticTypeInfo<SamplerUniform>().guid;
        break;
    }

    return std::optional<edt::GUID>();
}

void Shader::UpdateUniforms()
{
    GLuint num_uniforms;

    {
        GLint num_uniforms_;
        glGetProgramiv(*program_, GL_ACTIVE_UNIFORMS, &num_uniforms_);
        [[unlikely]] if (num_uniforms_ < 1)
        {
            return;
        }
        num_uniforms = static_cast<GLuint>(num_uniforms_);
    }

    GLint max_name_legth;
    glGetProgramiv(*program_, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_name_legth);

    std::string name_buffer_heap;
    constexpr GLsizei name_buffer_size_stack = 64;
    GLchar name_buffer_stack[name_buffer_size_stack];

    GLsizei name_buffer_size;
    char* name_buffer;

    if (max_name_legth < name_buffer_size_stack)
    {
        name_buffer = name_buffer_stack;
        name_buffer_size = name_buffer_size_stack;
    }
    else
    {
        name_buffer_heap.resize(static_cast<size_t>(max_name_legth));
        name_buffer = name_buffer_heap.data();
        name_buffer_size = static_cast<GLsizei>(max_name_legth);
    }

    std::vector<ShaderUniform> uniforms;
    uniforms.reserve(num_uniforms);
    for (GLuint i = 0; i != num_uniforms; ++i)
    {
        GLint variable_size;
        GLenum glsl_type;
        GLsizei actual_name_length;
        glGetActiveUniform(
            *program_,
            i,
            name_buffer_size,
            &actual_name_length,
            &variable_size,
            &glsl_type,
            name_buffer);

        const std::string_view variable_name_view(name_buffer, static_cast<size_t>(actual_name_length));
        const Name variable_name(variable_name_view);

        const std::optional<edt::GUID> cpp_type = ConvertGlType(glsl_type);
        if (!cpp_type)
        {
            fmt::print("Skip variable {} in \"{}\" - unsupported type", variable_name_view, path_.string());
            continue;
        }

        // find existing variable
        auto found_uniform_it = std::find_if(
            uniforms_.begin(),
            uniforms_.end(),
            [&](const ShaderUniform& u)
            {
                return u.GetName() == variable_name;
            });

        if (found_uniform_it != uniforms_.end())
        {
            uniforms.push_back(std::move(*found_uniform_it));
            // the previous value can be saved only if variable has the same type
            if (*cpp_type != found_uniform_it->GetTypeGUID())
            {
                uniforms.back().SetType(*cpp_type);
            }
        }
        else
        {
            uniforms.emplace_back();
            auto& uniform = uniforms.back();
            uniform.SetName(variable_name);
            uniform.SetType(*cpp_type);
        }

        const GLint location = glGetUniformLocation(*program_, variable_name.GetView().data());
        uniforms.back().SetLocation(static_cast<uint32_t>(location));
    }

    std::swap(uniforms, uniforms_);

    for (size_t i = 0; i < uniforms_.size(); ++i)
    {
        ShaderUniform& uniform = uniforms_[i];
        constexpr edt::GUID sampler_uniform_guid = cppreflection::GetStaticTypeInfo<SamplerUniform>().guid;
        if (uniform.GetTypeGUID() == sampler_uniform_guid)
        {
            UniformHandle handle{static_cast<uint32_t>(i), uniform.GetName()};
            auto sampler = GetUniformValue<SamplerUniform>(handle);
            sampler.sampler_index = static_cast<uint8_t>(i);
            uniform.SetValue(std::span(reinterpret_cast<const uint8_t*>(&sampler), sizeof(sampler)));
        }
    }
}

}  // namespace klgl
