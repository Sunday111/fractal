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
#include "opengl/debug/annotations.hpp"
#include "opengl/debug/gl_debug_messenger.hpp"
#include "reflection/register_types.hpp"
#include "shader/shader.hpp"
#include "window.hpp"
#include "wrap/wrap_glfw.hpp"
#include "wrap/wrap_imgui.hpp"

int InitializeGLAD_impl()
{
    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    return 42;
}

void InitializeGLAD()
{
    [[maybe_unused]] static int once = InitializeGLAD_impl();
}

int main([[maybe_unused]] int argc, char** argv)
{
    const std::filesystem::path exe_file = std::filesystem::path(argv[0]);
    RegisterReflectionTypes();

    const auto content_dir = exe_file.parent_path() / "content";
    const auto shaders_dir = content_dir / "shaders";
    Shader::shaders_dir_ = content_dir / "shaders";

    const double min_x = -2.0f;
    const double max_x = 0.47f;
    const double min_y = -1.12f;
    const double max_y = 1.12f;
    Eigen::Vector2d camera{min_x + (max_x - min_x) / 2, min_y + (max_y - min_y) / 2};
    Eigen::Vector3f color_seed{0.3f, 0.3f, 0.3f};

    GlfwState glfw_state;
    glfw_state.Initialize();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

    std::unique_ptr<Window> window;
    {
        ui32 window_width = 800;
        ui32 window_height = 800;
        if (GLFWmonitor* monitor = glfwGetPrimaryMonitor())
        {
            float x_scale, y_scale;
            glfwGetMonitorContentScale(monitor, &x_scale, &y_scale);
            window_width = static_cast<ui32>(static_cast<float>(window_width) * x_scale);
            window_height = static_cast<ui32>(static_cast<float>(window_height) * y_scale);
        }

        window = std::make_unique<Window>(window_width, window_height);
    }

    // GLAD can be initialized only when glfw has window context
    window->MakeContextCurrent();
    InitializeGLAD();

    GlDebugMessenger::Start();

    glfwSwapInterval(0);
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window->GetGlfwWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 130");

    if (GLFWmonitor* monitor = glfwGetPrimaryMonitor())
    {
        float xscale, yscale;
        glfwGetMonitorContentScale(monitor, &xscale, &yscale);

        ImGui::GetStyle().ScaleAllSizes(2);
        ImGuiIO& io = ImGui::GetIO();

        ImFontConfig font_config{};
        font_config.SizePixels = 13 * xscale;
        io.Fonts->AddFontDefault(&font_config);
    }

    auto shader = std::make_unique<Shader>("simple.shader.json");
    shader->Use();

    const std::array<Eigen::Vector3f, 6> vertices{
        Eigen::Vector3f{-1.f, -1.f, 0.0f},
        Eigen::Vector3f{1.f, -1.f, 0.0f},
        Eigen::Vector3f{-1.f, 1.f, 0.0f},

        Eigen::Vector3f{1.f, -1.f, 0.0f},
        Eigen::Vector3f{-1.f, 1.f, 0.0f},
        Eigen::Vector3f{1.f, 1.f, 0.0f}};

    auto vao = OpenGl::GenVertexArray();
    auto vbo = OpenGl::GenBuffer();
    OpenGl::BindVertexArray(vao);
    OpenGl::BindBuffer(GL_ARRAY_BUFFER, vbo);
    OpenGl::BufferData(GL_ARRAY_BUFFER, std::span{vertices}, GL_STATIC_DRAW);
    OpenGl::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), 0);
    OpenGl::EnableVertexAttribArray(0);

    auto pos_loc = shader->GetUniform("uCameraPos");
    auto scale_loc = shader->GetUniform("uScale");
    auto color_seed_loc = shader->GetUniform("uColorSeed");
    auto viewport_size_loc = shader->GetUniform("uViewportSize");

    constexpr double scale_factor = 0.95f;
    int scale_i = 0;

    auto prev_frame_time = std::chrono::high_resolution_clock::now();
    while (!window->ShouldClose())
    {
        ScopeAnnotation frame_annotation("Frame");
        const auto current_frame_time = std::chrono::high_resolution_clock::now();
        // const auto frame_delta_time =
        //     std::chrono::duration<float, std::chrono::seconds::period>(current_frame_time - prev_frame_time).count();

        OpenGl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        ImGui::Begin("Parameters");
        ImGui::SliderFloat3("Color Seed", color_seed.data(), 0.0f, 1.0f, "%.12f");

        if (ImGui::CollapsingHeader("Camera"))
        {
            ImGui::InputDouble("x", &camera.x(), 0.0, 0.0, "%.12f");
            ImGui::InputDouble("y", &camera.y(), 0.0, 0.0, "%.12f");
            ImGui::InputInt("Zoom", &scale_i);
        }

        ImGui::End();

        auto is_key_down = [&](auto key)
        {
            return glfwGetKey(window->GetGlfwWindow(), key) == GLFW_PRESS;
        };

        if (is_key_down(GLFW_KEY_E)) scale_i += 1;
        if (is_key_down(GLFW_KEY_Q)) scale_i = std::max(scale_i - 1, 0);
        const double scale = std::pow(scale_factor, scale_i);
        const double x_range = (max_x - min_x) * scale;
        const double y_range = (max_y - min_y) * scale;
        if (is_key_down(GLFW_KEY_W)) camera.y() += y_range * 0.01f;
        if (is_key_down(GLFW_KEY_S)) camera.y() -= y_range * 0.01f;
        if (is_key_down(GLFW_KEY_A)) camera.x() -= x_range * 0.01f;
        if (is_key_down(GLFW_KEY_D)) camera.x() += x_range * 0.01f;

        Eigen::Vector2f camera_f = camera.cast<float>();
        shader->SetUniform(pos_loc, camera_f);
        shader->SetUniform(scale_loc, static_cast<float>(scale));
        shader->SetUniform(color_seed_loc, color_seed);
        shader->SetUniform(
            viewport_size_loc,
            Eigen::Vector2f(static_cast<float>(window->GetWidth()), static_cast<float>(window->GetHeight())));
        shader->SendUniforms();
        shader->Use();

        OpenGl::BindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        {
            ScopeAnnotation imgui_render("ImGUI");
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        window->SwapBuffers();
        glfwPollEvents();
        prev_frame_time = current_frame_time;
    }

    return 0;
}