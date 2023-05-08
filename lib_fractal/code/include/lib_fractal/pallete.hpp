#pragma once

#include "lib_fractal/color.hpp"

namespace lib_fractal::pallete
{

template <size_t max_iterations>
constexpr Color MakeColorForIteration(size_t iteration)
{
    if (iteration == 0)
    {
        return Color::Fill(255);
    }

    if (iteration > 999)
    {
        return Color::Fill(0);
    }

    auto ch = [&]<size_t index>(std::index_sequence<index>)
    {
        constexpr size_t offset = max_iterations / 3;
        constexpr size_t center = 300 + (offset * (2 * index + 3) / 2) % max_iterations;
        constexpr size_t max_dist = center > max_iterations - center ? center : max_iterations - center;
        const size_t dist = center > iteration ? center - iteration : iteration - center;
        const float rel_dist = static_cast<float>(dist) / max_dist;
        return static_cast<uint8_t>(rel_dist * 255);
    };

    return Color{ch(std::index_sequence<0>{}), ch(std::index_sequence<1>{}), ch(std::index_sequence<2>{})};
}

template <size_t colors_count>
constexpr auto MakePallete()
{
    std::array<Color, colors_count> result{};
    for (size_t i = 0; i != colors_count; ++i)
    {
        result[i] = MakeColorForIteration<colors_count>(i);
    }

    return result;
}
}  // namespace lib_fractal::pallete
