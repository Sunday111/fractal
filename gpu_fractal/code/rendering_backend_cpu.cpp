#include "rendering_backend_cpu.hpp"

#include <cassert>

#include "klgl/application.hpp"
#include "klgl/mesh/mesh_data.hpp"
#include "klgl/shader/shader.hpp"
#include "klgl/texture/texture.hpp"
#include "klgl/window.hpp"
#include "mandelbrot.hpp"
#include "mesh_vertex.hpp"

using namespace klgl;

FractalCPURenderingThread::FractalCPURenderingThread(boost::lockfree::queue<ThreadTask*>& task_queue)
    : task_queue_(task_queue)
{
    thread_ = std::thread(&FractalCPURenderingThread::thread_main, this);
}

FractalCPURenderingThread::~FractalCPURenderingThread()
{
    must_stop_ = true;
    thread_.join();
}

Eigen::Vector3<uint8_t> FractalCPURenderingThread::ColorForIteration(ThreadTask& task, size_t iteration)
{
    if (iteration == task.iterations) return {0, 0, 0};

    const size_t segments_count = task.colors.size() - 1;
    const size_t iterations_per_segment = task.iterations / segments_count;
    const size_t k = iteration * segments_count;
    const size_t first_color_index = k / task.iterations;
    auto color_a = task.colors[first_color_index].cast<float>();
    auto color_b = task.colors[first_color_index + 1].cast<float>();
    float p = float(k % iterations_per_segment) / iterations_per_segment;
    return (color_a + (color_b - color_a) * p).cast<uint8_t>();
}

using FractalFunction = size_t (*)(const Float& x, const Float& y, size_t iterations);

template <size_t bits_count>
FractalFunction WrapFractalFunction()
{
    if constexpr (bits_count == 64)
    {
        return [](const Float& x, const Float& y, const size_t iterations)
        {
            return MandelbrotLoop<double>(static_cast<double>(x), static_cast<double>(y), iterations);
        };
    }
    else
    {
        return [](const Float& x, const Float& y, const size_t iterations)
        {
            using SF = boost::multiprecision::number<boost::multiprecision::cpp_dec_float<bits_count>>;
            return MandelbrotLoop<SF>(static_cast<SF>(x), static_cast<SF>(y), iterations);
        };
    }
}

static FractalFunction SelectFractalFunction(size_t bits_count)
{
    // clang-format off
    switch (bits_count)
    {
        case 65: return WrapFractalFunction<65>();
        case 66: return WrapFractalFunction<66>();
        case 67: return WrapFractalFunction<67>();
        case 68: return WrapFractalFunction<68>();
        case 69: return WrapFractalFunction<69>();
        case 60: return WrapFractalFunction<60>();
        case 71: return WrapFractalFunction<71>();
        case 72: return WrapFractalFunction<72>();
        case 73: return WrapFractalFunction<73>();
        case 74: return WrapFractalFunction<74>();
        case 75: return WrapFractalFunction<75>();
        case 76: return WrapFractalFunction<76>();
        case 77: return WrapFractalFunction<77>();
        case 78: return WrapFractalFunction<78>();
        case 79: return WrapFractalFunction<79>();
        case 80: return WrapFractalFunction<80>();
        case 81: return WrapFractalFunction<81>();
        case 82: return WrapFractalFunction<82>();
        case 83: return WrapFractalFunction<83>();
        case 84: return WrapFractalFunction<84>();
        case 85: return WrapFractalFunction<85>();
        case 86: return WrapFractalFunction<86>();
        case 87: return WrapFractalFunction<87>();
        case 88: return WrapFractalFunction<88>();
        case 89: return WrapFractalFunction<89>();
        case 90: return WrapFractalFunction<90>();
        case 91: return WrapFractalFunction<91>();
        case 92: return WrapFractalFunction<92>();
        case 93: return WrapFractalFunction<93>();
        case 94: return WrapFractalFunction<94>();
        case 95: return WrapFractalFunction<95>();
        case 96: return WrapFractalFunction<96>();
        case 97: return WrapFractalFunction<97>();
        case 98: return WrapFractalFunction<98>();
        case 99: return WrapFractalFunction<99>();
        default: return WrapFractalFunction<64>();
    }
    // clang-format on
}

void FractalCPURenderingThread::do_task(ThreadTask& task)
{
    task.pixels.resize(task.region_screen_size.x() * task.region_screen_size.y());
    const auto ff = SelectFractalFunction(task.float_bits_count);
    for (size_t y = 0; y != task.region_screen_size.y(); ++y)
    {
        if (task.cancelled)
        {
            break;
        }

        Float py = task.world_start_point.y() + task.world_step_per_pixel.y() * y;
        for (size_t x = 0; x != task.region_screen_size.x(); ++x)
        {
            Float px = task.world_start_point.x() + task.world_step_per_pixel.x() * x;
            const size_t iterations = ff(px, py, 1000);
            const auto color = ColorForIteration(task, iterations);
            task.pixels[y * task.region_screen_size.x() + x] = color;
        }
    }

    task.completed = true;
}

void FractalCPURenderingThread::thread_main()
{
    while (!must_stop_)
    {
        ThreadTask* task = nullptr;
        if (!task_queue_.pop(task))
        {
            continue;
        }

        do_task(*task);
    }
}

FractalRenderingBackendCPU::FractalRenderingBackendCPU(klgl::Application& app, FractalSettings& settings)
    : app_(app),
      settings_(settings),
      task_queue_(100)
{
    render_texture_shader = std::make_unique<Shader>("just_texture.shader.json");
    texture_loc = render_texture_shader->GetUniform("uTexture");

    const std::array<MeshVertex, 4> vertices{
        {{.position = {1.0f, 1.0f}, .tex_coord = {1.0f, 1.0f}},
         {.position = {1.0f, -1.0f}, .tex_coord = {1.0f, 0.0f}},
         {.position = {-1.0f, -1.0f}, .tex_coord = {0.0f, 0.0f}},
         {.position = {-1.0f, 1.0f}, .tex_coord = {0.0f, 1.0f}}}};
    const std::array<uint32_t, 6> indices{0, 1, 3, 1, 2, 3};

    // Make mesh
    quad_mesh = MeshOpenGL::MakeFromData<MeshVertex>(std::span{vertices}, std::span{indices});
    quad_mesh->Bind();
    RegisterAttribute<&MeshVertex::position>(0, false);
    RegisterAttribute<&MeshVertex::tex_coord>(1, false);

    CreateTexture();
    workers_.resize(10);

    for (auto& region : workers_)
    {
        if (!region)
        {
            region = std::make_unique<FractalCPURenderingThread>(task_queue_);
        }
    }
}

FractalRenderingBackendCPU::~FractalRenderingBackendCPU() = default;

void FractalRenderingBackendCPU::Draw()
{
    texture->Bind();

    for (auto& task : tasks_)
    {
        if (task->cancelled)
        {
            continue;
        }

        if (task->completed)
        {
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexSubImage2D(
                GL_TEXTURE_2D,
                0,
                static_cast<GLint>(task->region_screen_location.x()),
                static_cast<GLint>(task->region_screen_location.y()),
                static_cast<GLsizei>(task->region_screen_size.x()),
                static_cast<GLsizei>(task->region_screen_size.y()),
                GL_RGB,
                GL_UNSIGNED_BYTE,
                task->pixels.data());
        }
    }

    tasks_.erase(
        std::remove_if(
            tasks_.begin(),
            tasks_.end(),
            [](const auto& task) -> bool
            {
                return task->completed;
            }),
        tasks_.end());

    render_texture_shader->Use();
    render_texture_shader->SetUniform(texture_loc, *texture);
    render_texture_shader->SendUniforms();
    quad_mesh->Bind();
    quad_mesh->Draw();
}

void FractalRenderingBackendCPU::PostDraw()
{
    if (settings_.settings_applied)
    {
        return;
    }

    task_queue_.consume_all(
        [&](auto& task)
        {
            task->completed = true;
        });

    bool has_pending_tasks = false;
    for (auto& task : tasks_)
    {
        task->cancelled = true;
        if (!task->completed)
        {
            has_pending_tasks = true;
        }
    }

    if (has_pending_tasks)
    {
        return;
    }

    // Recreate texture to match window size
    CreateTexture();

    auto get_part = [](size_t full_size, size_t parts_count, size_t part_index)
    {
        const size_t min_part = full_size / parts_count;
        if (part_index == 0 && parts_count > 1)
        {
            return min_part + full_size % parts_count;
        }

        return min_part;
    };

    auto scale = settings_.GetScale();
    auto scaled_coord_range = settings_.global_coord_range * scale;

    const auto screenf = Vector2f(texture->GetSize().cast<Float>());
    const auto world_step_per_pixel = scaled_coord_range / screenf;
    const Vector2f world_start_location = settings_.camera - scaled_coord_range / 2;

    size_t location_y = 0;
    for (size_t ry = 0; ry != chunk_rows; ++ry)
    {
        size_t location_x = 0;
        const size_t region_height = get_part(texture->GetHeight(), chunk_rows, ry);
        for (size_t rx = 0; rx != chunk_cols; ++rx)
        {
            const size_t region_width = get_part(texture->GetWidth(), chunk_cols, rx);
            const Eigen::Vector2<size_t> region_screen_location{location_x, location_y};
            const Eigen::Vector2<size_t> region_screen_size{region_width, region_height};
            const auto region_world_start_location =
                world_start_location + Vector2f(region_screen_location.cast<Float>()) * world_step_per_pixel;

            {
                auto task = std::make_unique<ThreadTask>();
                task->iterations = 1000;
                task->float_bits_count = settings_.float_bits_count;
                task->colors.clear();
                task->world_start_point = region_world_start_location;
                task->world_step_per_pixel = world_step_per_pixel;
                task->region_screen_location = region_screen_location;
                task->region_screen_size = region_screen_size;
                for (auto& color : settings_.colors)
                {
                    task->colors.push_back((color * 255.f).cast<uint8_t>());
                }

                tasks_.push_back(std::move(task));
                task_queue_.push(tasks_.back().get());
            }

            location_x += region_width;
        }
        location_y += region_height;
    }

    settings_.settings_applied = true;
}

void FractalRenderingBackendCPU::CreateTexture()
{
    size_t window_width = app_.GetWindow().GetWidth();
    size_t window_height = app_.GetWindow().GetHeight();
    if (!texture || texture->GetWidth() != window_width || texture->GetHeight() != window_height)
    {
        texture = Texture::CreateEmpty(window_width, window_height);
    }
}
