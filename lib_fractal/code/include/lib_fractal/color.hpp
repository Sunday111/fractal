#pragma once

#include <cstdint>

namespace lib_fractal
{
struct Color
{
    uint8_t r;
    uint8_t g;
    uint8_t b;

    static constexpr Color Fill(uint8_t v)
    {
        return Color{v, v, v};
    }
};
}  // namespace lib_fractal