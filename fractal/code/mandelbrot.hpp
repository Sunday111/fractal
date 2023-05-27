#pragma once

#include <cstddef>

template <typename T>
inline size_t MandelbrotLoop(const T& x0, const T& y0, size_t max_iterations)
{
    T x2{0};
    T y2{0};
    T w{0};
    size_t iteration = 0;

    while (x2 + y2 <= 4 && iteration != max_iterations)
    {
        auto x = x2 - y2 + x0;
        auto y = w - x2 - y2 + y0;
        x2 = x * x;
        y2 = y * y;
        w = x + y;
        w *= w;

        ++iteration;
    }

    return iteration;
}
