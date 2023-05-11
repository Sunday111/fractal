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
#include "klgl/application.hpp"
#include "klgl/mesh/mesh_data.hpp"
#include "klgl/opengl/debug/annotations.hpp"
#include "klgl/opengl/gl_api.hpp"
#include "klgl/reflection/register_types.hpp"
#include "klgl/shader/shader.hpp"
#include "klgl/window.hpp"
#include "klgl/wrap/wrap_glfw.hpp"
#include "klgl/wrap/wrap_imgui.hpp"

using namespace klgl;

class FractalApp : public Application
{
public:
    using Super = Application;

    constexpr static double scale_factor = 0.95;
    constexpr static float pan_step = 0.1f;
    constexpr static size_t colors_count = 10;

    virtual void Initialize() override;
    virtual void Tick() override;

private:
    Eigen::Vector2d global_min_coord;
    Eigen::Vector2d global_max_coord;
    Eigen::Vector2d global_coord_range;
    Eigen::Vector2d camera;
    UniformHandle pos_loc;
    UniformHandle scale_loc;
    UniformHandle viewport_size_loc;
    int scale_i = 0;
    int color_seed = 1234;

    std::unique_ptr<Shader> shader;
    std::unique_ptr<MeshOpenGL> quad_mesh;
    std::array<Eigen::Vector3f, colors_count> colors;
    std::array<UniformHandle, colors_count> colors_uniforms;
};

void FractalApp::Initialize()
{
    Super::Initialize();

    const auto content_dir = GetExecutableDir() / "content";
    const auto shaders_dir = content_dir / "shaders";
    Shader::shaders_dir_ = shaders_dir;

    global_min_coord = {-2.0, -1.12};
    global_max_coord = {0.47, 1.12};
    global_coord_range = global_max_coord - global_min_coord;
    camera = global_min_coord + global_coord_range / 2;

    shader = std::make_unique<Shader>("simple.shader.json");
    shader->Use();

    pos_loc = shader->GetUniform("uCameraPos");
    scale_loc = shader->GetUniform("uScale");
    viewport_size_loc = shader->GetUniform("uViewportSize");

    std::string tmp;
    for (size_t i = 0; i != colors_uniforms.size(); ++i)
    {
        tmp.clear();
        fmt::format_to(std::back_inserter(tmp), "uColorTable[{}]", i);
        colors_uniforms[i] = shader->GetUniform(tmp.data());
    }

    MeshData mesh_data = MeshData::MakeIndexedQuad();
    quad_mesh = MeshOpenGL::MakeFromData(mesh_data);
    OpenGl::VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(mesh_data.vertices[0]), nullptr);
    OpenGl::EnableVertexAttribArray(0);
}

void FractalApp::Tick()
{
    Super::Tick();

    Window& window = GetWindow();
    if (window.IsKeyPressed(GLFW_KEY_E)) scale_i += 1;
    if (window.IsKeyPressed(GLFW_KEY_Q)) scale_i = std::max(scale_i - 1, 0);
    const double scale = std::pow(scale_factor, scale_i);
    auto scaled_coord_range = global_coord_range * scale;
    if (window.IsKeyPressed(GLFW_KEY_W)) camera.y() += scaled_coord_range.y() * pan_step;
    if (window.IsKeyPressed(GLFW_KEY_S)) camera.y() -= scaled_coord_range.y() * pan_step;
    if (window.IsKeyPressed(GLFW_KEY_A)) camera.x() -= scaled_coord_range.x() * pan_step;
    if (window.IsKeyPressed(GLFW_KEY_D)) camera.x() += scaled_coord_range.x() * pan_step;

    const Eigen::Vector2f camera_f = camera.cast<float>();
    shader->SetUniform(pos_loc, camera_f);
    shader->SetUniform(scale_loc, static_cast<float>(scale));
    shader->SetUniform(viewport_size_loc, window.GetSize2f());
    for (size_t color_index = 0; color_index != colors_count; ++color_index)
    {
        shader->SetUniform(colors_uniforms[color_index], colors[color_index]);
    }
    shader->SendUniforms();
    shader->Use();

    quad_mesh->Draw();

    ImGui::Begin("Parameters");

    if (ImGui::CollapsingHeader("Colors"))
    {
        for (size_t color_index = 0; color_index != colors.size(); ++color_index)
        {
            if (color_index)
            {
                ImGui::SameLine();
            }

            constexpr int color_edit_flags =
                ImGuiColorEditFlags_DefaultOptions_ | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel;
            auto& color = colors[color_index];
            ImGui::PushID(&color);
            ImGui::ColorEdit3("Color", color.data(), color_edit_flags);
            ImGui::PopID();
        }

        ImGui::InputInt("Color Seed: 1234", &color_seed);
        ImGui::SameLine();
        if (ImGui::Button("Randomize"))
        {
            std::mt19937 rnd(static_cast<unsigned>(color_seed));
            std::uniform_real_distribution<float> color_distr(0, 1.0f);
            for (auto& color : colors)
            {
                for (float& v : color)
                {
                    v = color_distr(rnd);
                }
            }
        }
    }

    if (ImGui::CollapsingHeader("Camera"))
    {
        ImGui::InputDouble("x", &camera.x(), 0.0, 0.0, "%.12f");
        ImGui::InputDouble("y", &camera.y(), 0.0, 0.0, "%.12f");
        ImGui::InputInt("Zoom", &scale_i);
    }

    if (ImGui::CollapsingHeader("Shader"))
    {
        shader->DrawDetails();
    }

    ImGui::End();
}

int main()
{
    FractalApp app;
    app.Run();
    return 0;
}
