#include <array>
#include <cstddef>
#include <filesystem>
#include <map>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "application.hpp"
#include "fmt/format.h"
#include "opengl/debug/annotations.hpp"
#include "opengl/debug/gl_debug_messenger.hpp"
#include "opengl/gl_api.hpp"
#include "reflection/register_types.hpp"
#include "shader/shader.hpp"
#include "window.hpp"
#include "wrap/wrap_glfw.hpp"
#include "wrap/wrap_imgui.hpp"

constexpr double min_x = -2.0;
constexpr double max_x = 0.47;
constexpr double min_y = -1.12;
constexpr double max_y = 1.12;
constexpr double scale_factor = 0.95;

class FractalApp : public klgl::Application
{
public:
    using Super = klgl::Application;

    virtual void Initialize() override;
    virtual void Tick() override;

private:
    Eigen::Vector2d camera;
    Eigen::Vector3f color_seed;
    GLuint vao;
    GLuint vbo;
    UniformHandle pos_loc;
    UniformHandle scale_loc;
    UniformHandle color_seed_loc;
    UniformHandle viewport_size_loc;
    int scale_i = 0;

    std::unique_ptr<Shader> shader;
};

void FractalApp::Initialize()
{
    Super::Initialize();

    const auto content_dir = GetExecutableDir() / "content";
    const auto shaders_dir = content_dir / "shaders";
    Shader::shaders_dir_ = shaders_dir;

    camera = {min_x + (max_x - min_x) / 2, min_y + (max_y - min_y) / 2};
    color_seed = {0.3f, 0.3f, 0.3f};

    shader = std::make_unique<Shader>("simple.shader.json");
    shader->Use();

    const std::array<Eigen::Vector3f, 6> vertices{
        Eigen::Vector3f{-1.f, -1.f, 0.0f},
        Eigen::Vector3f{1.f, -1.f, 0.0f},
        Eigen::Vector3f{-1.f, 1.f, 0.0f},

        Eigen::Vector3f{1.f, -1.f, 0.0f},
        Eigen::Vector3f{-1.f, 1.f, 0.0f},
        Eigen::Vector3f{1.f, 1.f, 0.0f}};

    vao = OpenGl::GenVertexArray();
    vbo = OpenGl::GenBuffer();
    OpenGl::BindVertexArray(vao);
    OpenGl::BindBuffer(GL_ARRAY_BUFFER, vbo);
    OpenGl::BufferData(GL_ARRAY_BUFFER, std::span{vertices}, GL_STATIC_DRAW);
    OpenGl::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), nullptr);
    OpenGl::EnableVertexAttribArray(0);

    pos_loc = shader->GetUniform("uCameraPos");
    scale_loc = shader->GetUniform("uScale");
    color_seed_loc = shader->GetUniform("uColorSeed");
    viewport_size_loc = shader->GetUniform("uViewportSize");
}

void FractalApp::Tick()
{
    Super::Tick();

    auto is_key_down = [&](auto key)
    {
        return glfwGetKey(GetWindow().GetGlfwWindow(), key) == GLFW_PRESS;
    };

    if (is_key_down(GLFW_KEY_E)) scale_i += 1;
    if (is_key_down(GLFW_KEY_Q)) scale_i = std::max(scale_i - 1, 0);
    const double scale = std::pow(scale_factor, scale_i);
    const double x_range = (max_x - min_x) * scale;
    const double y_range = (max_y - min_y) * scale;
    if (is_key_down(GLFW_KEY_W)) camera.y() += y_range * 0.01;
    if (is_key_down(GLFW_KEY_S)) camera.y() -= y_range * 0.01;
    if (is_key_down(GLFW_KEY_A)) camera.x() -= x_range * 0.01;
    if (is_key_down(GLFW_KEY_D)) camera.x() += x_range * 0.01;

    Eigen::Vector2f camera_f = camera.cast<float>();
    shader->SetUniform(pos_loc, camera_f);
    shader->SetUniform(scale_loc, static_cast<float>(scale));
    shader->SetUniform(color_seed_loc, color_seed);
    shader->SetUniform(
        viewport_size_loc,
        Eigen::Vector2f(static_cast<float>(GetWindow().GetWidth()), static_cast<float>(GetWindow().GetHeight())));
    shader->SendUniforms();
    shader->Use();

    OpenGl::BindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    ImGui::Begin("Parameters");
    ImGui::SliderFloat3("Color Seed", color_seed.data(), 0.0f, 1.0f, "%.12f");

    if (ImGui::CollapsingHeader("Camera"))
    {
        ImGui::InputDouble("x", &camera.x(), 0.0, 0.0, "%.12f");
        ImGui::InputDouble("y", &camera.y(), 0.0, 0.0, "%.12f");
        ImGui::InputInt("Zoom", &scale_i);
    }

    ImGui::End();
}

int main()
{
    FractalApp app;
    app.Run();
    return 0;
}