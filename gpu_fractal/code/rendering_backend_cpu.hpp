#pragma once

#include <atomic>
#include <span>
#include <thread>
#include <vector>

#include "fractal_settings.hpp"
#include "klgl/shader/uniform_handle.hpp"
#include "klgl/wrap/wrap_eigen.hpp"

namespace klgl
{
class Application;
class Texture;
class Shader;
struct MeshOpenGL;
}  // namespace klgl

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

    FractalCPURenderingThread(std::shared_ptr<const CommonSettings> common_settings);
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

    void SetTask(Eigen::Vector2<size_t> region_location, Eigen::Vector2<size_t> region_size);

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
    Eigen::Vector2<size_t> location;
    Eigen::Vector2<size_t> size;
    std::atomic<State> state_ = State::Pending;
    std::vector<Eigen::Vector3<uint8_t>> pixels;
    std::shared_ptr<const CommonSettings> settings;
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
    std::shared_ptr<FractalCPURenderingThread::CommonSettings> threads_settings_;
};
