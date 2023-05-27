#include "fractal_settings.hpp"

#include "vector.hpp"

void FractalSettings::Update()
{
    settings_applied = false;

    scale_ = boost::multiprecision::pow(scale_factor_, zoom_);

    Vector2f offset;
    offset.Fill(scale_ * 2.0);

    if (width_ != height_)
    {
        if (width_ > height_)
        {
            offset.x() *= Float(width_) / height_;
        }
        else
        {
            offset.y() *= Float(height_) / width_;
        }
    }

    min_coord_ = camera_;
    min_coord_ -= offset;
    max_coord_ = camera_;
    max_coord_ += offset;
    coord_range_ = offset * 2;

    step_per_pixel_ = coord_range_;
    step_per_pixel_.x() /= width_;
    step_per_pixel_.y() /= height_;
}

void FractalSettings::MoveCameraToPixel(const size_t x, const size_t y, bool flip_y)
{
    Vector2f p;
    p.x() = x;
    p.x() /= width_;

    if (flip_y)
    {
        p.y() = height_;
        p.y() -= y;
    }
    else
    {
        p.y() = y;
    }
    p.y() /= height_;
    p *= coord_range_;
    p += min_coord_;
    SetCamera(p);
}

void FractalSettings::PanCamera(const PanCameraOpts opts)
{
    if (opts.dir_x == 0 && opts.dir_y == 0)
    {
        return;
    }

    Float abs_delta = pan_speed_ * opts.dt;
    std::array<int, 2> dirs{opts.dir_x, opts.dir_y};

    for (size_t index = 0; index != 2; ++index)
    {
        if (const int dir = dirs[index]; dir != 0)
        {
            if (dir > 0)
            {
                camera_[index] += abs_delta * coord_range_[index];
            }
            else
            {
                camera_[index] -= abs_delta * coord_range_[index];
            }
        }
    }

    Update();
}

Vector2f FractalSettings::GetCoordAtPixel(const size_t x, const size_t y) const
{
    Vector2f p = min_coord_;
    p.x() += step_per_pixel_.x() * x;
    p.y() += step_per_pixel_.y() * y;
    return p;
}
