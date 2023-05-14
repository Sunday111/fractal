#pragma once

#include <cstddef>

#include "float.hpp"

inline size_t MandelbrotLoop(const Float& x0, const Float& y0, size_t max_iterations)
{
    Float x(0.0);
    Float y(0.0);
    size_t iteration = 0;

    const Float limit(4.0);
    const Float two(2.0);

    while (iteration != max_iterations)
    {
        auto xs = x * x;
        auto ys = y * y;
        if ((xs + ys).compare(limit) > 0)
        {
            break;
        }

        Float x_temp = xs - ys + x0;
        y = two * x * y + y0;
        x = x_temp;
        ++iteration;
    }

    return iteration;
}
