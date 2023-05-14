#pragma once

#include <array>
#include <concepts>
#include <type_traits>

#include "float.hpp"
#include "klgl/wrap/wrap_eigen.hpp"

template <typename T, size_t N>
class Vector
{
public:
    static constexpr size_t kSize = N;

    Vector() = default;

    explicit Vector(const Eigen::Vector<T, N>& v)
    {
        for (size_t index = 0; index != N; ++index)
        {
            data[index] = v[index];
        }
    }

    explicit Vector(const T& value)
    {
        for (size_t index = 0; index != N; ++index)
        {
            data[index] = value;
        }
    }

    T& operator[](const size_t index)
    {
        return data[index];
    }

    const T& operator[](const size_t index) const
    {
        return data[index];
    }

    auto& operator+=(const Vector<T, N>& another)
    {
        for (size_t index = 0; index != N; ++index)
        {
            data[index] += another.data[index];
        }
        return *this;
    }

    auto operator+(const Vector<T, N>& another) const
    {
        auto copy = *this;
        copy += another;
        return copy;
    }

    auto& operator+=(const T& another)
    {
        *this += Vector(another);
        return *this;
    }

    auto operator+(const T& another) const
    {
        auto copy = *this;
        copy += another;
        return copy;
    }

    auto& operator-=(const Vector<T, N>& another)
    {
        for (size_t index = 0; index != N; ++index)
        {
            data[index] -= another.data[index];
        }
        return *this;
    }

    auto operator-(const Vector<T, N>& another) const
    {
        auto copy = *this;
        copy -= another;
        return copy;
    }

    auto& operator-=(const T& another)
    {
        *this -= Vector(another);
        return *this;
    }

    auto operator-(const T& another) const
    {
        auto copy = *this;
        copy -= another;
        return copy;
    }

    auto& operator/=(const Vector<T, N>& another)
    {
        for (size_t index = 0; index != N; ++index)
        {
            data[index] /= another.data[index];
        }
        return *this;
    }

    auto operator/(const Vector<T, N>& another) const
    {
        auto copy = *this;
        copy /= another;
        return copy;
    }

    auto& operator/=(const T& another)
    {
        *this /= Vector(another);
        return *this;
    }

    auto operator/(const T& another) const
    {
        auto copy = *this;
        copy /= another;
        return copy;
    }

    auto& operator*=(const Vector<T, N>& another)
    {
        for (size_t index = 0; index != N; ++index)
        {
            data[index] *= another.data[index];
        }
        return *this;
    }

    auto operator*(const Vector<T, N>& another) const
    {
        auto copy = *this;
        copy *= another;
        return copy;
    }

    auto& operator*=(const T& another)
    {
        *this *= Vector(another);
        return *this;
    }

    auto operator*(const T& another) const
    {
        auto copy = *this;
        copy *= another;
        return copy;
    }

    auto operator-() const
    {
        auto copy = *this;
        for (auto& v : copy.data)
        {
            v = -v;
        }
        return copy;
    }

    auto& x()
    {
        return data[0];
    }

    const auto& x() const
    {
        return data[0];
    }

    auto& y()
    {
        return data[1];
    }

    const auto& y() const
    {
        return data[1];
    }

    auto& z()
    {
        return data[2];
    }

    const auto& z() const
    {
        return data[2];
    }

    std::array<T, N> data;
};

using Vector2f = Vector<Float, 2>;
