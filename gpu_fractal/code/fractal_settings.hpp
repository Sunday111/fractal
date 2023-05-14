#pragma once

#include "klgl/wrap/wrap_eigen.hpp"
#include "vector.hpp"

struct FractalSettings
{
    constexpr static size_t colors_count = 10;
    constexpr static double scale_factor = 0.95;
    constexpr static float pan_step = 0.1f;

    FractalSettings()
    {
        global_min_coord.x() = -2.0;
        global_min_coord.y() = -1.12;
        global_max_coord.x() = 0.47;
        global_max_coord.y() = 1.12;
        global_coord_range = global_max_coord - global_min_coord;
        camera = global_min_coord + global_coord_range / 2;
    }

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

    void ShiftCameraX(const Float& delta)
    {
        camera.x() += delta;
        settings_applied = false;
    }

    void ShiftCameraY(const Float& delta)
    {
        camera.y() += delta;
        settings_applied = false;
    }

    Vector2f global_min_coord;
    Vector2f global_max_coord;
    Vector2f global_coord_range;
    Vector2f camera;

    int scale_i = 0;
    int color_seed = 1234;
    std::array<Eigen::Vector3f, colors_count> colors;
    bool settings_applied = false;
};
