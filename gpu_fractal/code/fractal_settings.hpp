#pragma once

#include "klgl/wrap/wrap_eigen.hpp"

struct FractalSettings
{
    constexpr static size_t colors_count = 10;
    constexpr static double scale_factor = 0.95;
    constexpr static float pan_step = 0.1f;

    FractalSettings()
    {
        global_min_coord = {-2.0, -1.12};
        global_max_coord = {0.47, 1.12};
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

    Eigen::Vector2d global_min_coord;
    Eigen::Vector2d global_max_coord;
    Eigen::Vector2d global_coord_range;

    Eigen::Vector2d camera;
    int scale_i = 0;
    int color_seed = 1234;
    std::array<Eigen::Vector3f, colors_count> colors;
    bool settings_applied = false;
};
