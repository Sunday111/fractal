#pragma once

#include "klgl/wrap/wrap_eigen.hpp"
#include "vector.hpp"

struct FractalSettings
{
    constexpr static size_t colors_count = 10;

    FractalSettings()
    {
        camera_.Fill(0);
        scale_factor_ = Float(0.95);
        pan_speed_ = 0.1;
        width_ = 800;
        height_ = 800;

        Update();
    }

    void IncrementScale()
    {
        ++zoom_;
        Update();
    }

    void DecrementScale()
    {
        if (zoom_ != 0)
        {
            --zoom_;
            Update();
        }
    }

    void ShiftCameraX(const Float& delta)
    {
        camera_.x() += delta;
        settings_applied = false;
    }

    void ShiftCameraY(const Float& delta)
    {
        camera_.y() += delta;
        settings_applied = false;
    }

    const Float& GetScale() const
    {
        return scale_;
    }

    void SetViewportSize(size_t width, size_t height)
    {
        if (width != width_ || height != height_)
        {
            width_ = width;
            height_ = height;
            Update();
        }
    }

    void SetZoom(uint16_t zoom)
    {
        if (zoom != zoom_)
        {
            zoom_ = zoom;
            Update();
        }
    }

    uint16_t GetZoom() const
    {
        return zoom_;
    }

    const Vector2f& GetCoordRange() const
    {
        return coord_range_;
    }

    const Vector2f& GetStepPerPixel() const
    {
        return step_per_pixel_;
    }

    Vector2f GetCoordAtPixel(size_t x, size_t y) const;

    void MoveCameraToPixel(const size_t x, const size_t y, bool flip_y);

    const Vector2f& GetCamera() const
    {
        return camera_;
    }

    void SetCamera(const Vector2f& camera, bool check_not_same = true)
    {
        if (check_not_same && camera.x() == camera_.x() && camera.y() == camera_.y())
        {
            return;
        }

        camera_ = camera;
        Update();
    }

    struct PanCameraOpts
    {
        float dt = 0.0f;
        int dir_x = 0;
        int dir_y = 0;
    };

    void PanCamera(const PanCameraOpts opts);

    int color_seed = 1234;
    std::array<Eigen::Vector3f, colors_count> colors;
    bool settings_applied = false;
    bool use_double = true;

private:
    void Update();

private:
    Float scale_;
    Float scale_factor_;
    uint16_t zoom_ = 0;
    Vector2f coord_range_;
    Vector2f min_coord_;
    Vector2f max_coord_;
    Vector2f step_per_pixel_;
    size_t width_;
    size_t height_;
    Float pan_speed_;
    Vector2f camera_;
};
