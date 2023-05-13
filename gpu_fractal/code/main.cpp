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
#include "lib_fractal/lib_fractal.hpp"

using namespace klgl;

struct FractalSettings
{
    constexpr static size_t colors_count = 10;

    void IncrementScale()
    {
        ++scale_i;
        settings_applied = false;
    }

    void DecrementScale()
    {
        if (scale_i != 0)
        {
            --scale_i;
            settings_applied = false;
        }
    }

    void ShiftCameraX(double delta)
    {
        camera.x() += delta;
        settings_applied = false;
    }
    void ShiftCameraY(double delta)
    {
        camera.y() += delta;
        settings_applied = false;
    }

    Eigen::Vector2d camera;
    int scale_i = 0;
    int color_seed = 1234;
    std::array<Eigen::Vector3f, colors_count> colors;
    bool settings_applied = false;
};

class FractalCPURenderingThread
{
public:
    struct CommonSettings
    {
        Eigen::Vector2d camera;
        Eigen::Vector2d range;
        std::vector<Eigen::Vector3<uint8_t>> colors;
        Eigen::Vector2<size_t> screen;
        size_t iterations;
    };

    enum class State
    {
        Pending,
        InProgress,
        MustStop
    };

    FractalCPURenderingThread(std::shared_ptr<const CommonSettings> common_settings)
    {
        settings = common_settings;
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
        return std::span{pixels};
    }

    void SetTask(Eigen::Vector2<size_t> region_location, Eigen::Vector2<size_t> region_size)
    {
        assert(state_ == State::Pending);
        location = region_location;
        size = region_size;
        state_ = State::InProgress;
    }

    Eigen::Vector2<size_t> GetSize() const
    {
        return size;
    }
    Eigen::Vector2<size_t> GetLocation() const
    {
        return location;
    }

private:
    Eigen::Vector3<uint8_t> ColorForIteration(size_t iteration) const
    {
        if (iteration == settings->iterations) return {0, 0, 0};

        const size_t segments_count = settings->colors.size() - 1;
        const size_t iterations_per_segment = settings->iterations / segments_count;
        const size_t k = iteration * segments_count;
        const size_t first_color_index = k / settings->iterations;
        auto color_a = settings->colors[first_color_index].cast<float>();
        auto color_b = settings->colors[first_color_index + 1].cast<float>();
        float p = float(k % iterations_per_segment) / iterations_per_segment;
        return (color_a + (color_b - color_a) * p).cast<uint8_t>();
    }

    void render()
    {
        pixels.resize(size.x() * size.y());

        const Eigen::Vector2d screend = settings->screen.cast<double>();

        const auto start_point = settings->camera - settings->range / 2;
        for (size_t y = 0; y != size.y(); ++y)
        {
            for (size_t x = 0; x != size.x(); ++x)
            {
                Eigen::Vector2d point = (location + Eigen::Vector2<size_t>{x, y}).cast<double>();
                point = start_point + point.cwiseProduct(settings->range).cwiseQuotient(screend);
                const size_t iterations = lib_fractal::MandelbrotLoop(point.x(), point.y(), 1000);
                const auto color = ColorForIteration(iterations);
                pixels[y * size.x() + x] = color;
            }
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
                render();
                auto expected_state = State::InProgress;
                state_.compare_exchange_strong(expected_state, State::Pending);
            }
        }
    }

private:
    std::thread thread_;
    std::mutex lock_;

    Eigen::Vector2<size_t> location;
    Eigen::Vector2<size_t> size;
    std::atomic<State> state_ = State::Pending;
    std::vector<Eigen::Vector3<uint8_t>> pixels;
    std::shared_ptr<const CommonSettings> settings;
};

struct FractalRegion
{
    bool has_new_data = false;
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

    virtual void Initialize() override;
    virtual void Tick(float dt) override;
    virtual void PostTick(float dt) override;

private:
    void MakeRegions();
    void CreateTextureForCpuRendering();

private:
    bool render_on_cpu = true;

    Eigen::Vector2d global_min_coord;
    Eigen::Vector2d global_max_coord;
    Eigen::Vector2d global_coord_range;
    FractalSettings settings;
    UniformHandle pos_loc;
    UniformHandle scale_loc;
    UniformHandle viewport_size_loc;

    std::unique_ptr<Shader> shader;
    std::unique_ptr<Shader> render_texture_shader;
    std::unique_ptr<MeshOpenGL> quad_mesh;
    std::array<UniformHandle, FractalSettings::colors_count> colors_uniforms;

    // CPU rendering
    std::vector<FractalRegion> regions;  // fractal regions that will be rendered in another thread
    std::unique_ptr<Texture> texture;
    UniformHandle texture_loc;
    std::shared_ptr<FractalCPURenderingThread::CommonSettings> threads_settings_;
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
    settings.camera = global_min_coord + global_coord_range / 2;

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

void FractalApp::Tick(float delta_time)
{
    Super::Tick(delta_time);

    Window& window = GetWindow();
    const double scale = std::pow(scale_factor, static_cast<float>(settings.scale_i));
    {
        if (window.IsKeyPressed(GLFW_KEY_E)) settings.IncrementScale();
        if (window.IsKeyPressed(GLFW_KEY_Q)) settings.DecrementScale();
        auto scaled_coord_range = global_coord_range * scale;
        auto frame_pan = pan_step * delta_time;
        if (window.IsKeyPressed(GLFW_KEY_W)) settings.ShiftCameraY(scaled_coord_range.y() * frame_pan);
        if (window.IsKeyPressed(GLFW_KEY_S)) settings.ShiftCameraY(-scaled_coord_range.y() * frame_pan);
        if (window.IsKeyPressed(GLFW_KEY_A)) settings.ShiftCameraX(-scaled_coord_range.x() * frame_pan);
        if (window.IsKeyPressed(GLFW_KEY_D)) settings.ShiftCameraX(scaled_coord_range.x() * frame_pan);
    }

    if (render_on_cpu)
    {
        texture->Bind();
        for (auto& region : regions)
        {
            if (region.has_new_data && region.thread->GetState() == FractalCPURenderingThread::State::Pending)
            {
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glTexSubImage2D(
                    GL_TEXTURE_2D,
                    0,
                    static_cast<GLint>(region.thread->GetLocation().x()),
                    static_cast<GLint>(region.thread->GetLocation().y()),
                    static_cast<GLsizei>(region.thread->GetSize().x()),
                    static_cast<GLsizei>(region.thread->GetSize().y()),
                    GL_RGB,
                    GL_UNSIGNED_BYTE,
                    region.thread->GetPixels().data());
                region.has_new_data = false;
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

    if (render_on_cpu && !settings.settings_applied)
    {
        bool all_pending = true;
        for (size_t ry = 0; ry != chunk_rows && all_pending; ++ry)
        {
            for (size_t rx = 0; rx != chunk_cols && all_pending; ++rx)
            {
                auto& region = regions[ry * chunk_cols + rx];
                if (region.thread->GetState() != FractalCPURenderingThread::State::Pending)
                {
                    all_pending = false;
                }
            }
        }

        if (!all_pending)
        {
            return;
        }

        // Recreate texture to match window size
        CreateTextureForCpuRendering();

        auto get_part = [](size_t full_size, size_t parts_count, size_t part_index)
        {
            const size_t min_part = full_size / parts_count;
            if (part_index == 0 && parts_count > 1)
            {
                return min_part + full_size % parts_count;
            }

            return min_part;
        };

        const double scale = std::pow(scale_factor, settings.scale_i);
        auto scaled_coord_range = global_coord_range * scale;
        threads_settings_->camera = settings.camera;
        threads_settings_->iterations = 1000;
        threads_settings_->range = scaled_coord_range;
        threads_settings_->screen = texture->GetSize();
        threads_settings_->colors.clear();
        for (auto& color : settings.colors)
        {
            threads_settings_->colors.push_back((color * 255.f).cast<uint8_t>());
        }

        size_t location_y = 0;
        for (size_t ry = 0; ry != chunk_rows; ++ry)
        {
            size_t location_x = 0;
            const size_t region_height = get_part(texture->GetHeight(), chunk_rows, ry);
            for (size_t rx = 0; rx != chunk_cols; ++rx)
            {
                auto& region = regions[ry * chunk_cols + rx];
                const size_t region_width = get_part(texture->GetWidth(), chunk_cols, rx);
                const Eigen::Vector2<size_t> region_location{location_x, location_y};
                const Eigen::Vector2<size_t> region_size{region_width, region_height};
                region.thread->SetTask(region_location, region_size);
                region.has_new_data = true;
                location_x += region_width;
            }
            location_y += region_height;
        }

        settings.settings_applied = true;
    }
}

void FractalApp::MakeRegions()
{
    CreateTextureForCpuRendering();
    threads_settings_ = std::make_unique<FractalCPURenderingThread::CommonSettings>();
    regions.resize(chunk_rows * chunk_cols);

    for (auto& region : regions)
    {
        if (!region.thread)
        {
            region.thread = std::make_unique<FractalCPURenderingThread>(threads_settings_);
        }
    }
}

void FractalApp::CreateTextureForCpuRendering()
{
    size_t window_width = GetWindow().GetWidth();
    size_t window_height = GetWindow().GetHeight();
    if (!texture || texture->GetWidth() != window_width || texture->GetHeight() != window_height)
    {
        texture = Texture::CreateEmpty(window_width, window_height);
    }
}

int main()
{
    FractalApp app;
    app.Run();
    return 0;
}
