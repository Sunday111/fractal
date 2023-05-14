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

#include "boost/multiprecision/cpp_dec_float.hpp"
#include "float.hpp"
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
#include "mesh_vertex.hpp"
#include "rendering_backend_cpu.hpp"
#include "rendering_backend_gpu.hpp"

using namespace klgl;

class FractalApp : public Application
{
public:
    using Super = Application;

    void DrawSettings();
    void RandomizeColors();

    virtual void Initialize() override;
    virtual void Tick(float dt) override;
    virtual void PostTick(float dt) override;

private:
    bool render_on_cpu = true;
    FractalSettings settings;
    std::unique_ptr<FractalRenderingBackendCPU> rendering_backend_cpu_;
    std::unique_ptr<FractalRenderingBackendGPU> rendering_backend_gpu_;
};

void FractalApp::Initialize()
{
    Super::Initialize();

    const auto content_dir = GetExecutableDir() / "content";
    const auto shaders_dir = content_dir / "shaders";
    Shader::shaders_dir_ = shaders_dir;
    RandomizeColors();
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
        if (window.IsKeyPressed(GLFW_KEY_S)) settings.ShiftCameraY(Float(1) - scaled_coord_range.y() * frame_pan);
        if (window.IsKeyPressed(GLFW_KEY_A)) settings.ShiftCameraX(Float(1) - scaled_coord_range.x() * frame_pan);
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
        if (!rendering_backend_gpu_)
        {
            rendering_backend_gpu_ = std::make_unique<FractalRenderingBackendGPU>(*this, settings);
        }
        rendering_backend_gpu_->Draw();
    }

    DrawSettings();

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

void FractalApp::DrawSettings()
{
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
            RandomizeColors();
            settings_changed = true;
        }
    }

    std::string tmp;
    auto float_input = [&](const char* title, Float& v)
    {
        tmp = v.str(std::numeric_limits<double>::digits10 + 3, std::ios_base::fixed);
        tmp.resize(1000);
        settings_changed |= ImGui::InputText(title, tmp.data(), 1000);
        v = Float(tmp.c_str());
    };

    if (ImGui::CollapsingHeader("Camera"))
    {
        float_input("x", settings.camera.x());
        float_input("y", settings.camera.y());
        settings_changed |= ImGui::InputInt("Zoom", &settings.scale_i);
    }

    if (settings_changed)
    {
        settings.settings_applied = false;
    }
}

void FractalApp::RandomizeColors()
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
}

int main()
{
    FractalApp app;
    app.Run();
    return 0;
}
