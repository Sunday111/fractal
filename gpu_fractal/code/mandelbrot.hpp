#pragma once

#include <cstddef>

template <typename T>
inline size_t MandelbrotLoop(const T& x0, const T& y0, size_t max_iterations)
{
    T x{0};
    T y{0};
    size_t iteration = 0;

    const T limit(4);
    const T two(2);

    while (iteration != max_iterations)
    {
        auto xs = x * x;
        auto ys = y * y;
        if (xs + ys >= limit)
        {
            break;
        }

        T x_temp = xs - ys + x0;
        y = two * x * y + y0;
        x = x_temp;
        ++iteration;
    }

    return iteration;
}
