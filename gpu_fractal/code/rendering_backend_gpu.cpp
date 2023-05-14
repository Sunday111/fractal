#include "rendering_backend_gpu.hpp"

#include "fmt/format.h"
#include "klgl/application.hpp"
#include "klgl/mesh/mesh_data.hpp"
#include "klgl/opengl/debug/annotations.hpp"
#include "klgl/reflection/register_types.hpp"
#include "klgl/shader/shader.hpp"
#include "klgl/window.hpp"
#include "mesh_vertex.hpp"

using namespace klgl;

FractalRenderingBackendGPU::FractalRenderingBackendGPU(klgl::Application& app, FractalSettings& settings)
    : app_(app),
      settings_(settings)
{
    shader_ = std::make_unique<Shader>("simple.shader.json");

    pos_loc = shader_->GetUniform("uCameraPos");
    scale_loc = shader_->GetUniform("uScale");
    viewport_size_loc = shader_->GetUniform("uViewportSize");

    size_t uniforms_count = colors_uniforms.size();
    for (size_t i = 0; i != uniforms_count; ++i)
    {
        std::string uniform_name = fmt::format("uColorTable[{}]", i);
        colors_uniforms[i] = shader_->GetUniform(uniform_name.data());
    }

    const std::array<MeshVertex, 4> vertices{
        {{.position = {1.0f, 1.0f}, .tex_coord = {1.0f, 1.0f}},
         {.position = {1.0f, -1.0f}, .tex_coord = {1.0f, 0.0f}},
         {.position = {-1.0f, -1.0f}, .tex_coord = {0.0f, 0.0f}},
         {.position = {-1.0f, 1.0f}, .tex_coord = {0.0f, 1.0f}}}};
    const std::array<uint32_t, 6> indices{0, 1, 3, 1, 2, 3};

    quad_mesh = MeshOpenGL::MakeFromData<MeshVertex>(std::span{vertices}, std::span{indices});
    quad_mesh->Bind();
    RegisterAttribute<&MeshVertex::position>(0, false);
    RegisterAttribute<&MeshVertex::tex_coord>(1, false);
}

void FractalRenderingBackendGPU::Draw()
{
    ScopeAnnotation annotation("Render fractal on gpu");
    const Eigen::Vector2f camera_f = settings_.camera.cast<float>();
    const double scale = std::pow(settings_.scale_factor, settings_.scale_i);
    shader_->SetUniform(pos_loc, camera_f);
    shader_->SetUniform(scale_loc, static_cast<float>(scale));
    shader_->SetUniform(viewport_size_loc, app_.GetWindow().GetSize2f());
    for (size_t color_index = 0; color_index != colors_uniforms.size(); ++color_index)
    {
        shader_->SetUniform(colors_uniforms[color_index], settings_.colors[color_index]);
    }
    shader_->SendUniforms();
    shader_->Use();
    quad_mesh->BindAndDraw();
}
