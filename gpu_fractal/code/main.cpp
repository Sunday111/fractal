#include <array>
#include <cstddef>
#include <filesystem>
#include <map>
#include <random>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "fmt/format.h"
#include "fractal_settings.hpp"
#include "klgl/application.hpp"
#include "klgl/mesh/mesh_data.hpp"
#include "klgl/opengl/debug/annotations.hpp"
#include "klgl/opengl/gl_api.hpp"
#include "klgl/reflection/register_types.hpp"
#include "klgl/shader/shader.hpp"
#include "klgl/texture/texture.hpp"
#include "klgl/window.hpp"
#include "klgl/wrap/wrap_glfw.hpp"
#include "klgl/wrap/wrap_imgui.hpp"
#include "lib_fractal/lib_fractal.hpp"
#include "mesh_vertex.hpp"
#include "rendering_backend_cpu.hpp"

using namespace klgl;

class FractalApp : public Application
{
public:
    using Super = Application;

    virtual void Initialize() override;
    virtual void Tick(float dt) override;
    virtual void PostTick(float dt) override;

private:
    bool render_on_cpu = true;

    FractalSettings settings;
    UniformHandle pos_loc;
    UniformHandle scale_loc;
    UniformHandle viewport_size_loc;

    std::unique_ptr<Shader> shader;
    std::unique_ptr<MeshOpenGL> quad_mesh;
    std::array<UniformHandle, FractalSettings::colors_count> colors_uniforms;

    std::unique_ptr<FractalRenderingBackendCPU> rendering_backend_cpu_;
};

void FractalApp::Initialize()
{
    Super::Initialize();

    const auto content_dir = GetExecutableDir() / "content";
    const auto shaders_dir = content_dir / "shaders";
    Shader::shaders_dir_ = shaders_dir;

    shader = std::make_unique<Shader>("simple.shader.json");

    pos_loc = shader->GetUniform("uCameraPos");
    scale_loc = shader->GetUniform("uScale");
    viewport_size_loc = shader->GetUniform("uViewportSize");

    size_t uniforms_count = colors_uniforms.size();
    for (size_t i = 0; i != uniforms_count; ++i)
    {
        std::string uniform_name = fmt::format("uColorTable[{}]", i);
        colors_uniforms[i] = shader->GetUniform(uniform_name.data());
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

void FractalApp::Tick(float delta_time)
{
    Super::Tick(delta_time);

    Window& window = GetWindow();
    const double scale = std::pow(settings.scale_factor, static_cast<float>(settings.scale_i));
    {
        if (window.IsKeyPressed(GLFW_KEY_E)) settings.IncrementScale();
        if (window.IsKeyPressed(GLFW_KEY_Q)) settings.DecrementScale();
        auto scaled_coord_range = settings.global_coord_range * scale;
        auto frame_pan = settings.pan_step * delta_time;
        if (window.IsKeyPressed(GLFW_KEY_W)) settings.ShiftCameraY(scaled_coord_range.y() * frame_pan);
        if (window.IsKeyPressed(GLFW_KEY_S)) settings.ShiftCameraY(-scaled_coord_range.y() * frame_pan);
        if (window.IsKeyPressed(GLFW_KEY_A)) settings.ShiftCameraX(-scaled_coord_range.x() * frame_pan);
        if (window.IsKeyPressed(GLFW_KEY_D)) settings.ShiftCameraX(scaled_coord_range.x() * frame_pan);
    }

    if (render_on_cpu)
    {
        if (!rendering_backend_cpu_)
        {
            rendering_backend_cpu_ = std::make_unique<FractalRenderingBackendCPU>(*this, settings);
        }
        rendering_backend_cpu_->Draw();
    }
    else
    {
        ScopeAnnotation annotation("Render fractal on gpu");
        const Eigen::Vector2f camera_f = settings.camera.cast<float>();
        shader->SetUniform(pos_loc, camera_f);
        shader->SetUniform(scale_loc, static_cast<float>(scale));
        shader->SetUniform(viewport_size_loc, window.GetSize2f());
        for (size_t color_index = 0; color_index != colors_uniforms.size(); ++color_index)
        {
            shader->SetUniform(colors_uniforms[color_index], settings.colors[color_index]);
        }
        shader->SendUniforms();
        shader->Use();
        quad_mesh->BindAndDraw();
    }

    bool settings_changed = false;
    ImGui::Begin("Parameters");

    if (ImGui::Checkbox("Render on cpu", &render_on_cpu))
    {
        settings.settings_applied = false;
    }

    if (ImGui::CollapsingHeader("Colors"))
    {
        for (size_t color_index = 0; color_index != settings.colors.size(); ++color_index)
        {
            if (color_index)
            {
                ImGui::SameLine();
            }

            constexpr int color_edit_flags =
                ImGuiColorEditFlags_DefaultOptions_ | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel;
            auto& color = settings.colors[color_index];
            ImGui::PushID(&color);
            settings_changed |= ImGui::ColorEdit3("Color", color.data(), color_edit_flags);
            ImGui::PopID();
        }

        ImGui::InputInt("Color Seed: 1234", &settings.color_seed);
        ImGui::SameLine();
        if (ImGui::Button("Randomize"))
        {
            std::mt19937 rnd(static_cast<unsigned>(settings.color_seed));
            std::uniform_real_distribution<float> color_distr(0, 1.0f);
            for (auto& color : settings.colors)
            {
                for (float& v : color)
                {
                    v = color_distr(rnd);
                }
            }
            settings_changed = true;
        }
    }

    if (ImGui::CollapsingHeader("Camera"))
    {
        settings_changed |= ImGui::InputDouble("x", &settings.camera.x(), 0.0, 0.0, "%.12f");
        settings_changed |= ImGui::InputDouble("y", &settings.camera.y(), 0.0, 0.0, "%.12f");
        settings_changed |= ImGui::InputInt("Zoom", &settings.scale_i);
    }

    if (ImGui::CollapsingHeader("Shader"))
    {
        shader->DrawDetails();
    }

    if (settings_changed)
    {
        settings.settings_applied = false;
    }

    ImGui::End();
}

void FractalApp::PostTick(float delta_time)
{
    Super::PostTick(delta_time);

    if (rendering_backend_cpu_)
    {
        rendering_backend_cpu_->PostDraw();
    }
}

int main()
{
    FractalApp app;
    app.Run();
    return 0;
}
