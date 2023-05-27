#include <array>
#include <cstddef>
#include <filesystem>
#include <map>
#include <optional>
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
#include "gui/fractal_gui.hpp"
#include "imgui.h"
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
#include "rendering_backend/rendering_backend_cpu.hpp"
#include "rendering_backend/rendering_backend_gpu.hpp"

using namespace klgl;

class FractalApp : public Application
{
public:
    using Super = Application;

    void DrawSettings(const float dt);
    void RandomizeColors();

    void Initialize() override;
    void Tick(float dt) override;
    void PostTick(float dt) override;

private:
    bool render_on_cpu = false;
    FractalSettings settings;
    std::unique_ptr<FractalRenderingBackend> rendering_backend_;
};

void FractalApp::Initialize()
{
    Super::Initialize();

    GetWindow().SetSize(1280, 720);
    GetWindow().SetTitle("Fractal");

    const auto content_dir = GetExecutableDir() / "content";
    const auto shaders_dir = content_dir / "shaders";
    Shader::shaders_dir_ = shaders_dir;
    RandomizeColors();
}

void FractalApp::Tick(float delta_time)
{
    Super::Tick(delta_time);

    // Update viewport size
    Window& window = GetWindow();
    settings.SetViewportSize(window.GetWidth(), window.GetHeight());

    if (rendering_backend_)
    {
        rendering_backend_->Draw();
    }

    DrawSettings(delta_time);
}

void FractalApp::PostTick(float delta_time)
{
    Super::PostTick(delta_time);

    if (rendering_backend_)
    {
        rendering_backend_->PostDraw();
    }
}

void FractalApp::DrawSettings(const float dt)
{
    ImGui::Begin("Parameters");

    if (!rendering_backend_ || ImGui::Checkbox("Render on cpu", &render_on_cpu))
    {
        if (render_on_cpu)
        {
            rendering_backend_ = std::make_unique<FractalRenderingBackendCPU>(*this, settings);
        }
        else
        {
            rendering_backend_ = std::make_unique<FractalRenderingBackendGPU>(*this, settings);
        }

        settings.settings_applied = false;
    }

    FractalGUI gui{settings};
    gui.Draw(dt);

    if (rendering_backend_)
    {
        rendering_backend_->DrawSettings();
    }

    ImGui::End();
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
    setenv("GALLIUM_DRIVER", "llvmpipe", 1);
    FractalApp app;
    app.Run();
    return 0;
}
