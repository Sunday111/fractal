#pragma once

#include <cstddef>

namespace lib_fractal
{

constexpr size_t MandelbrotLoop(double x0, double y0, size_t max_iterations)
{
    double x = 0.0;
    double y = 0.0;
    size_t iteration = 0;
    while (x * x + y * y <= 4.0 && iteration != max_iterations)
    {
        double x_temp = x * x - y * y + x0;
        y = 2 * x * y + y0;
        x = x_temp;
        ++iteration;
    }

    return iteration;
}

}  // namespace lib_fractal