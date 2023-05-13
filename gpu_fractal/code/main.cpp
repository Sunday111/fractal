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
#include "klgl/template/class_member_traits.hpp"
#include "klgl/template/member_offset.hpp"
#include "klgl/template/type_to_gl_type.hpp"
#include "klgl/texture/texture.hpp"
#include "klgl/window.hpp"
#include "klgl/wrap/wrap_glfw.hpp"
#include "klgl/wrap/wrap_imgui.hpp"

using namespace klgl;

class FractalCPURenderingThread
{
public:
    struct Task
    {
        Eigen::Vector3<uint8_t> color;
        Eigen::Vector2<size_t> size;
    };

    enum class State
    {
        Pending,
        InProgress,
        MustStop
    };

    FractalCPURenderingThread()
    {
        thread_ = std::thread(&FractalCPURenderingThread::thread_main, this);
    }

    ~FractalCPURenderingThread()
    {
        state_ = State::MustStop;
        thread_.join();
    }

    State GetState() const
    {
        return state_;
    }

    std::span<const Eigen::Vector3<uint8_t>> GetPixels() const
    {
        return std::span{pixels_};
    }

    void SetTask(Task task)
    {
        assert(state_ == State::Pending);
        task_ = task;
        state_ = State::InProgress;
    }

private:
    void render(Task task)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100 + rand() % 2000));

        // const size_t row_size = sizeof(pixels_[0]) * task.size.x();
        // size_t row_size_aligned = row_size;
        // if (row_size_aligned % 4)
        // {
        //     row_size_aligned += 4 - (row_size_aligned % 4);
        // }

        Eigen::Vector3<uint8_t> color = task.color;
        color.x() += rand() % 255;
        color.y() += rand() % 255;
        color.z() += rand() % 255;
        pixels_.resize(task.size.x() * task.size.y());
        for (auto& pixel : pixels_)
        {
            pixel = color;
        }
    }

    void thread_main()
    {
        while (state_ != State::MustStop)
        {
            // Spin lock waiting for a task
            while (state_ == State::Pending)
            {
            }

            if (state_ == State::InProgress)
            {
                render(task_);
                state_ = State::Pending;
            }
        }
    }

private:
    std::thread thread_;
    std::mutex lock_;

    std::atomic<State> state_ = State::Pending;
    std::vector<Eigen::Vector3<uint8_t>> pixels_;
    Task task_;
};

struct FractalRegion
{
    bool has_new_data = false;
    Eigen::Vector2<size_t> location;
    Eigen::Vector2<size_t> size;
    std::unique_ptr<FractalCPURenderingThread> thread;
};

class FractalApp : public Application
{
public:
    using Super = Application;

    constexpr static size_t chunk_rows = 3;
    constexpr static size_t chunk_cols = 3;
    constexpr static double scale_factor = 0.95;
    constexpr static float pan_step = 0.1f;
    constexpr static size_t colors_count = 10;

    virtual void Initialize() override;
    virtual void Tick() override;
    virtual void PostTick() override;

private:
    void MakeRegions();

private:
    bool render_on_cpu = true;

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
    std::unique_ptr<Shader> render_texture_shader;
    std::unique_ptr<MeshOpenGL> quad_mesh;
    std::array<Eigen::Vector3f, colors_count> colors;
    std::array<UniformHandle, colors_count> colors_uniforms;

    // CPU rendering
    std::vector<FractalRegion> regions;  // fractal regions that will be rendered in another thread
    std::unique_ptr<Texture> texture;
    UniformHandle texture_loc;
};

template <auto MemberVariablePtr>
void RegisterAttribute(GLuint location, bool normalized)
{
    using MemberTraits = ClassMemberTraits<decltype(MemberVariablePtr)>;
    using GlTypeTraits = TypeToGlType<typename MemberTraits::Member>;
    const size_t vertex_stride = sizeof(typename MemberTraits::Class);
    const size_t member_stride = MemberOffset<MemberVariablePtr>();
    OpenGl::VertexAttribPointer(
        location,
        GlTypeTraits::Size,
        GlTypeTraits::Type,
        normalized,
        vertex_stride,
        reinterpret_cast<void*>(member_stride));
    OpenGl::EnableVertexAttribArray(location);
}

struct MeshVertex
{
    Eigen::Vector2f position;
    Eigen::Vector2f tex_coord;
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

    pos_loc = shader->GetUniform("uCameraPos");
    scale_loc = shader->GetUniform("uScale");
    viewport_size_loc = shader->GetUniform("uViewportSize");

    render_texture_shader = std::make_unique<Shader>("just_texture.shader.json");
    texture_loc = render_texture_shader->GetUniform("uTexture");

    std::string tmp;
    for (size_t i = 0; i != colors_uniforms.size(); ++i)
    {
        tmp.clear();
        fmt::format_to(std::back_inserter(tmp), "uColorTable[{}]", i);
        colors_uniforms[i] = shader->GetUniform(tmp.data());
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

    MakeRegions();
}

void FractalApp::Tick()
{
    Super::Tick();

    Window& window = GetWindow();
    const double scale = std::pow(scale_factor, scale_i);
    {
        if (window.IsKeyPressed(GLFW_KEY_E)) scale_i += 1;
        if (window.IsKeyPressed(GLFW_KEY_Q)) scale_i = std::max(scale_i - 1, 0);
        auto scaled_coord_range = global_coord_range * scale;
        if (window.IsKeyPressed(GLFW_KEY_W)) camera.y() += scaled_coord_range.y() * pan_step;
        if (window.IsKeyPressed(GLFW_KEY_S)) camera.y() -= scaled_coord_range.y() * pan_step;
        if (window.IsKeyPressed(GLFW_KEY_A)) camera.x() -= scaled_coord_range.x() * pan_step;
        if (window.IsKeyPressed(GLFW_KEY_D)) camera.x() += scaled_coord_range.x() * pan_step;
    }

    if (render_on_cpu)
    {
        // Draw regions to final texture

        texture->Bind();
        for (auto& region : regions)
        {
            if (region.has_new_data && region.thread->GetState() == FractalCPURenderingThread::State::Pending)
            {
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glTexSubImage2D(
                    GL_TEXTURE_2D,
                    0,
                    static_cast<GLint>(region.location.x()),
                    static_cast<GLint>(region.location.y()),
                    static_cast<GLsizei>(region.size.x()),
                    static_cast<GLsizei>(region.size.y()),
                    GL_RGB,
                    GL_UNSIGNED_BYTE,
                    region.thread->GetPixels().data());
            }
        }

        render_texture_shader->Use();
        render_texture_shader->SetUniform(texture_loc, *texture);
        render_texture_shader->SendUniforms();
        quad_mesh->Bind();
        quad_mesh->Draw();
    }
    else
    {
        ScopeAnnotation annotation("Render fractal on gpu");
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
        quad_mesh->BindAndDraw();
    }

    ImGui::Begin("Parameters");

    ImGui::Checkbox("Render on cpu", &render_on_cpu);

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

void FractalApp::PostTick()
{
    Super::PostTick();

    if (render_on_cpu)
    {
        for (size_t ry = 0; ry != chunk_rows; ++ry)
        {
            for (size_t rx = 0; rx != chunk_cols; ++rx)
            {
                auto& region = regions[ry * chunk_cols + rx];
                if (region.thread->GetState() == FractalCPURenderingThread::State::Pending)
                {
                    auto r = static_cast<uint8_t>(ry * 127);
                    auto g = static_cast<uint8_t>((2 - rx) * 127);
                    region.thread->SetTask(FractalCPURenderingThread::Task{
                        .color = Eigen::Vector3<uint8_t>{r, g, 0},
                        .size = region.size});
                    region.has_new_data = true;
                }
            }
        }
    }
}

void FractalApp::MakeRegions()
{
    size_t window_width = GetWindow().GetWidth();
    size_t window_height = GetWindow().GetHeight();
    texture = Texture::CreateEmpty(window_width, window_height);
    regions.resize(chunk_rows * chunk_cols);

    for (auto& region : regions)
    {
        if (!region.thread)
        {
            region.thread = std::make_unique<FractalCPURenderingThread>();
        }
    }

    auto get_part = [](size_t full_size, size_t parts_count, size_t part_index)
    {
        const size_t min_part = full_size / parts_count;
        if (part_index == 0 && parts_count > 1)
        {
            return min_part + full_size % parts_count;
        }

        return min_part;
    };

    size_t location_y = 0;
    for (size_t ry = 0; ry != chunk_rows; ++ry)
    {
        size_t location_x = 0;
        const size_t region_height = get_part(window_height, chunk_rows, ry);
        for (size_t rx = 0; rx != chunk_cols; ++rx)
        {
            auto& region = regions[ry * chunk_cols + rx];
            const size_t region_width = get_part(window_width, chunk_cols, rx);
            region.location = {location_x, location_y};
            region.size = {region_width, region_height};
            location_x += region_width;
        }
        location_y += region_height;
    }
}

int main()
{
    FractalApp app;
    app.Run();
    return 0;
}
