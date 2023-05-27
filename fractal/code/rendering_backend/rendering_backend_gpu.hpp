#pragma once

#include <array>
#include <memory>
#include <span>

#include "fractal_settings.hpp"
#include "klgl/shader/uniform_handle.hpp"
#include "rendering_backend.hpp"

namespace klgl
{
class Application;
class Shader;
struct MeshOpenGL;
}  // namespace klgl

class FractalRenderingBackendGPU : public FractalRenderingBackend
{
public:
    FractalRenderingBackendGPU(klgl::Application& app, FractalSettings& settings);

    void Draw() override;

private:
    klgl::Application& app_;
    FractalSettings& settings_;
    std::unique_ptr<klgl::Shader> shader_;
    klgl::UniformHandle pos_loc;
    klgl::UniformHandle scale_loc;
    klgl::UniformHandle viewport_size_loc;

    std::unique_ptr<klgl::MeshOpenGL> quad_mesh;
    std::array<klgl::UniformHandle, FractalSettings::colors_count> colors_uniforms;
};
