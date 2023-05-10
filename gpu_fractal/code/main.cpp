#include <array>
#include <cstddef>
#include <filesystem>
#include <map>
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

    virtual void Initialize() override;
    virtual void Tick() override;

private:
    Eigen::Vector2d global_min_coord;
    Eigen::Vector2d global_max_coord;
    Eigen::Vector2d global_coord_range;
    Eigen::Vector2d camera;
    Eigen::Vector3f color_seed;
    UniformHandle pos_loc;
    UniformHandle scale_loc;
    UniformHandle color_seed_loc;
    UniformHandle viewport_size_loc;
    int scale_i = 0;

    std::unique_ptr<Shader> shader;
    std::unique_ptr<MeshOpenGL> quad_mesh;
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
    color_seed = {0.3f, 0.3f, 0.3f};

    shader = std::make_unique<Shader>("simple.shader.json");
    shader->Use();

    pos_loc = shader->GetUniform("uCameraPos");
    scale_loc = shader->GetUniform("uScale");
    color_seed_loc = shader->GetUniform("uColorSeed");
    viewport_size_loc = shader->GetUniform("uViewportSize");

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
    shader->SetUniform(color_seed_loc, color_seed);
    shader->SetUniform(viewport_size_loc, window.GetSize2f());
    shader->SendUniforms();
    shader->Use();

    quad_mesh->Draw();

    ImGui::Begin("Parameters");
    ImGui::SliderFloat3("Color Seed", color_seed.data(), 0.0f, 1.0f, "%.12f");

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
