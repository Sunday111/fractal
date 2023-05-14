#pragma once

#include <atomic>
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
};

class FractalCPURenderingThread
{
public:
    enum class State
    {
        Pending,
        InProgress,
        MustStop
    };

    FractalCPURenderingThread();
    ~FractalCPURenderingThread();

    State GetState() const
    {
        return state_;
    }

    bool HasNewData() const
    {
        return has_new_data_ && state_ == State::Pending;
    }

    std::span<const Eigen::Vector3<uint8_t>> ConsumePixels();

    void SetTask(std::unique_ptr<ThreadTask> task);

    Eigen::Vector2<size_t> GetSize() const
    {
        return task->region_screen_size;
    }
    Eigen::Vector2<size_t> GetLocation() const
    {
        return task->region_screen_location;
    }

private:
    Eigen::Vector3<uint8_t> ColorForIteration(size_t iteration) const;
    void render();
    void thread_main();

private:
    std::thread thread_;
    std::atomic<State> state_ = State::Pending;
    std::vector<Eigen::Vector3<uint8_t>> pixels;
    std::unique_ptr<ThreadTask> task;
    bool has_new_data_ = false;
};

class FractalRenderingBackendCPU
{
public:
    constexpr static size_t chunk_rows = 3;
    constexpr static size_t chunk_cols = 3;

    FractalRenderingBackendCPU(klgl::Application& app, FractalSettings& settings);
    ~FractalRenderingBackendCPU();

    void Draw();
    void PostDraw();
    void CreateTexture();

private:
    klgl::Application& app_;
    FractalSettings& settings_;

    std::unique_ptr<klgl::Shader> render_texture_shader;
    klgl::UniformHandle texture_loc;
    std::unique_ptr<klgl::MeshOpenGL> quad_mesh;

    std::vector<std::unique_ptr<FractalCPURenderingThread>> regions;
    std::unique_ptr<klgl::Texture> texture;
};
