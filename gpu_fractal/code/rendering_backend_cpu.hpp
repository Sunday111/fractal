#pragma once

#include <atomic>
#include <chrono>
#include <optional>
#include <span>
#include <thread>
#include <vector>

#include "boost/lockfree/queue.hpp"
#include "fractal_settings.hpp"
#include "klgl/shader/uniform_handle.hpp"
#include "klgl/wrap/wrap_eigen.hpp"
#include "wrap/wrap_boost_asio.hpp"

namespace klgl
{
class Application;
class Texture;
class Shader;
struct MeshOpenGL;
}  // namespace klgl

struct ThreadTask
{
    Vector2f world_start_point;
    Vector2f world_step_per_pixel;
    Eigen::Vector2<size_t> region_screen_location;
    Eigen::Vector2<size_t> region_screen_size;
    std::vector<Eigen::Vector3<uint8_t>> colors;
    size_t iterations;
    size_t float_bits_count = 64;
    std::vector<uint16_t> pixels_iterations;
    std::atomic_bool completed = false;
    std::atomic_bool cancelled = false;
    std::atomic<uint16_t> rows_completed = 0;
    float task_duration_seconds = 0.0f;
};

class FractalCPURenderingThread
{
public:
    FractalCPURenderingThread(boost::lockfree::queue<ThreadTask*>& task_queue);
    ~FractalCPURenderingThread();
    static Eigen::Vector3<uint8_t> ColorForIteration(ThreadTask& task, size_t iteration);

private:
    static void do_task(ThreadTask& task);
    void thread_main();

private:
    std::thread thread_;
    std::atomic_bool must_stop_ = false;
    boost::lockfree::queue<ThreadTask*>& task_queue_;
};

class FractalRenderingBackendCPU
{
public:
    constexpr static size_t chunk_rows = 10;
    constexpr static size_t chunk_cols = 10;

    FractalRenderingBackendCPU(klgl::Application& app, FractalSettings& settings);
    ~FractalRenderingBackendCPU();

    void Draw();
    void PostDraw();
    void CreateTexture();

    std::optional<float> TakePreviousFrameDuration()
    {
        std::optional<float> r;
        prev_frame_duration_.swap(r);
        return r;
    }

private:
    void CancelAllTasks();
    void HandleCompletedTasks();
    bool HasTasksInProgress() const;
    void StartNewFractalFrame();

private:
    klgl::Application& app_;
    FractalSettings& settings_;

    std::optional<float> prev_frame_duration_;
    std::optional<float> current_frame_duration_;

    std::vector<std::unique_ptr<ThreadTask>> tasks_;
    std::vector<std::unique_ptr<ThreadTask>> ready_for_display_;
    boost::lockfree::queue<ThreadTask*> task_queue_;

    std::unique_ptr<klgl::Shader> render_texture_shader;
    klgl::UniformHandle texture_loc;
    std::unique_ptr<klgl::MeshOpenGL> quad_mesh;

    std::vector<std::unique_ptr<FractalCPURenderingThread>> workers_;
    std::unique_ptr<klgl::Texture> texture;
};
