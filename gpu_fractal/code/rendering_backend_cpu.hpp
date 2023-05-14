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

struct FractalThreadSettings
{
    Vector2f camera;
    Vector2f range;
    std::vector<Eigen::Vector3<uint8_t>> colors;
    Eigen::Vector2<size_t> screen;
    size_t iterations;
    size_t float_bits_count = 64;
};

class ThreadJob
{
public:
    enum class State
    {
        Pending,
        InProgress,
        Complete,
        Cancelled
    };

    std::atomic<State> state;
    std::vector<Eigen::Vector3<uint8_t>> data;
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

    FractalCPURenderingThread(std::shared_ptr<const FractalThreadSettings> common_settings);
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

    void SetTask(
        const Vector2f& start_point,
        const Vector2f& step_size,
        Eigen::Vector2<size_t> region_location,
        Eigen::Vector2<size_t> region_size);

    Eigen::Vector2<size_t> GetSize() const
    {
        return size;
    }
    Eigen::Vector2<size_t> GetLocation() const
    {
        return location;
    }

private:
    Eigen::Vector3<uint8_t> ColorForIteration(size_t iteration) const;
    void render();
    void thread_main();

private:
    std::thread thread_;
    Vector2f start_point;
    Vector2f step_size;
    Eigen::Vector2<size_t> location;
    Eigen::Vector2<size_t> size;
    std::atomic<State> state_ = State::Pending;
    std::vector<Eigen::Vector3<uint8_t>> pixels;
    std::shared_ptr<const FractalThreadSettings> settings;
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
    std::vector<std::unique_ptr<ThreadJob>> jobs_;
    boost::lockfree::queue<ThreadJob*> job_queue_;

    klgl::Application& app_;
    FractalSettings& settings_;

    std::unique_ptr<klgl::Shader> render_texture_shader;
    klgl::UniformHandle texture_loc;
    std::unique_ptr<klgl::MeshOpenGL> quad_mesh;

    std::vector<std::unique_ptr<FractalCPURenderingThread>> regions;
    std::unique_ptr<klgl::Texture> texture;
    std::shared_ptr<FractalThreadSettings> threads_settings_;
};
