#include "rendering_backend_cpu.hpp"

#include <cassert>

#include "klgl/application.hpp"
#include "klgl/mesh/mesh_data.hpp"
#include "klgl/shader/shader.hpp"
#include "klgl/texture/texture.hpp"
#include "klgl/window.hpp"
#include "mandelbrot.hpp"
#include "mesh_vertex.hpp"

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
    const float p = static_cast<float>(k % iterations_per_segment) / static_cast<float>(iterations_per_segment);
    return (color_a + (color_b - color_a) * p).cast<uint8_t>();
}

using FractalFunction = size_t (*)(const Float& x, const Float& y, size_t iterations);

static FractalFunction SelectFractalFunction(bool use_double)
{
    if (use_double)
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
            return MandelbrotLoop<Float>(x, y, iterations);
        };
    }
}

void FractalCPURenderingThread::do_task(ThreadTask& task)
{
    auto start_time = std::chrono::high_resolution_clock::now();
    task.pixels_iterations.resize(task.region_screen_size.prod());
    const auto ff = SelectFractalFunction(task.use_double);
    for (size_t y = 0; y != task.region_screen_size.y(); ++y)
    {
        task.rows_completed = static_cast<uint16_t>(y);

        if (task.cancelled)
        {
            break;
        }

        Float py = task.world_start_point.y() + task.world_step_per_pixel.y() * y;
        for (size_t x = 0; x != task.region_screen_size.x(); ++x)
        {
            Float px = task.world_start_point.x() + task.world_step_per_pixel.x() * x;
            const auto iterations = static_cast<uint16_t>(ff(px, py, 1000));
            task.pixels_iterations[y * task.region_screen_size.x() + x] = iterations;
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    task.task_duration_seconds =
        std::chrono::duration_cast<std::chrono::duration<float>>(end_time - start_time).count();
    task.completed = true;
}

void FractalCPURenderingThread::thread_main()
{
    while (!must_stop_)
    {
        ThreadTask* task = nullptr;
        if (task_queue_.pop(task))
        {
            do_task(*task);
        }
    }
}

FractalRenderingBackendCPU::FractalRenderingBackendCPU(klgl::Application& app, FractalSettings& settings)
    : app_(app),
      settings_(settings),
      task_queue_(200)
{
    render_texture_shader = std::make_unique<klgl::Shader>("just_texture.shader.json");
    texture_loc = render_texture_shader->GetUniform("uTexture");

    const std::array<MeshVertex, 4> vertices{
        {{.position = {1.0f, 1.0f}, .tex_coord = {1.0f, 1.0f}},
         {.position = {1.0f, -1.0f}, .tex_coord = {1.0f, 0.0f}},
         {.position = {-1.0f, -1.0f}, .tex_coord = {0.0f, 0.0f}},
         {.position = {-1.0f, 1.0f}, .tex_coord = {0.0f, 1.0f}}}};
    const std::array<uint32_t, 6> indices{0, 1, 3, 1, 2, 3};

    // Make mesh
    quad_mesh = klgl::MeshOpenGL::MakeFromData<MeshVertex>(std::span{vertices}, std::span{indices});
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

FractalRenderingBackendCPU::~FractalRenderingBackendCPU()
{
    CancelAllTasks();
}

void FractalRenderingBackendCPU::Draw()
{
    texture->Bind();

    while (!ready_for_display_.empty())
    {
        auto task = std::move(ready_for_display_.back());
        ready_for_display_.pop_back();

        std::vector<Eigen::Vector3<uint8_t>> pixels;
        pixels.reserve(task->pixels_iterations.size());
        for (uint16_t iterations : task->pixels_iterations)
        {
            pixels.emplace_back(FractalCPURenderingThread::ColorForIteration(*task, iterations));
        }

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
            pixels.data());
    }

    render_texture_shader->Use();
    render_texture_shader->SetUniform(texture_loc, *texture);
    render_texture_shader->SendUniforms();
    quad_mesh->Bind();
    quad_mesh->Draw();
}

void FractalRenderingBackendCPU::CancelAllTasks()
{
    task_queue_.consume_all(
        [&](auto& task)
        {
            task->cancelled = true;
            task->completed = true;
        });
}

void FractalRenderingBackendCPU::HandleCompletedTasks()
{
    // Consume complete tasks
    for (auto& task : tasks_)
    {
        if (task->completed)
        {
            if (!task->cancelled)
            {
                *current_frame_duration_ += task->task_duration_seconds;
                ready_for_display_.emplace_back(std::move(task));
            }
            task = nullptr;
        }
    }

    tasks_.erase(
        std::remove_if(
            tasks_.begin(),
            tasks_.end(),
            [&](auto& task)
            {
                return task == nullptr;
            }),
        tasks_.end());
}

bool FractalRenderingBackendCPU::HasTasksInProgress() const
{
    if (!task_queue_.empty())
    {
        return true;
    }

    for (auto& task : tasks_)
    {
        if (!task->completed)
        {
            return true;
        }
    }

    return false;
}

void FractalRenderingBackendCPU::StartNewFractalFrame()
{
    assert(task_queue_.empty());

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

    std::vector<std::unique_ptr<ThreadTask>> temp_tasks_;

    size_t location_y = 0;
    for (size_t ry = 0; ry != kChunkWidth; ++ry)
    {
        size_t location_x = 0;
        const size_t region_height = get_part(texture->GetHeight(), kChunkWidth, ry);
        for (size_t rx = 0; rx != kChunkHeight; ++rx)
        {
            const size_t region_width = get_part(texture->GetWidth(), kChunkHeight, rx);
            const Eigen::Vector2<size_t> region_screen_location{location_x, location_y};
            const Eigen::Vector2<size_t> region_screen_size{region_width, region_height};

            {
                auto task = std::make_unique<ThreadTask>();
                task->iterations = 1000;
                task->use_double = settings_.use_double;
                task->colors.clear();
                task->world_start_point = settings_.GetCoordAtPixel(location_x, location_y);
                task->world_step_per_pixel = settings_.GetStepPerPixel();
                task->region_screen_location = region_screen_location;
                task->region_screen_size = region_screen_size;
                for (auto& color : settings_.colors)
                {
                    task->colors.push_back((color * 255.f).cast<uint8_t>());
                }

                temp_tasks_.push_back(std::move(task));
            }

            location_x += region_width;
        }
        location_y += region_height;
    }

    // prioritize tasks that closer to the center of texture
    std::ranges::sort(
        temp_tasks_,
        [&](const std::unique_ptr<ThreadTask>& a, const std::unique_ptr<ThreadTask>& b)
        {
            const auto screeni = texture->GetSize();
            Eigen::Vector2i dist_a = a->region_screen_location.cast<int>() - (screeni / 2).cast<int>();
            Eigen::Vector2i dist_b = b->region_screen_location.cast<int>() - (screeni / 2).cast<int>();
            return dist_a.squaredNorm() < dist_b.squaredNorm();
        });

    for (auto& task : temp_tasks_)
    {
        ThreadTask* task_ptr = task.get();
        tasks_.push_back(std::move(task));
        [[maybe_unused]] const bool added = task_queue_.push(task_ptr);
        assert(added);
    }
}

void FractalRenderingBackendCPU::PostDraw()
{
    HandleCompletedTasks();

    bool in_progress = HasTasksInProgress();

    if (!in_progress)
    {
        if (current_frame_duration_.has_value())
        {
            prev_frame_duration_ = current_frame_duration_;
            current_frame_duration_ = std::nullopt;
        }
    }

    // Settings has changed
    if (!settings_.settings_applied)
    {
        // But previous were not rendered yet
        if (in_progress)
        {
            // So cancell all current tasks and try again on the next frame
            CancelAllTasks();
        }
        else
        {
            // Or start new frames
            StartNewFractalFrame();
            settings_.settings_applied = true;
            current_frame_duration_ = 0.0f;
        }
    }
}

void FractalRenderingBackendCPU::CreateTexture()
{
    size_t window_width = app_.GetWindow().GetWidth();
    size_t window_height = app_.GetWindow().GetHeight();
    if (!texture || texture->GetWidth() != window_width || texture->GetHeight() != window_height)
    {
        texture = klgl::Texture::CreateEmpty(window_width, window_height);
    }
}
