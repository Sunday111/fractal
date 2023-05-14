#pragma once

#include <cstddef>

#include "float.hpp"

inline size_t MandelbrotLoop(const Float& x0, const Float& y0, size_t max_iterations)
{
    Float x(0.0);
    Float y(0.0);
    size_t iteration = 0;

    while ((x * x + y * y).compare(Float(4.0)) >= 0 && iteration != max_iterations)
    {
        Float x_temp = x * x - y * y + x0;
        y = Float(2) * x * y + y0;
        x = x_temp;
        ++iteration;
    }

    return iteration;
}

template <typename T>
inline size_t MandelbrotLoop(const T& x0, const T& y0, size_t max_iterations)
{
    T x(0.0);
    T y(0.0);
    size_t iteration = 0;

    while ((x * x + y * y) < 4.0 && iteration != max_iterations)
    {
        T x_temp = x * x - y * y + x0;
        y = T(2) * x * y + y0;
        x = x_temp;
        ++iteration;
    }

    return iteration;
}
