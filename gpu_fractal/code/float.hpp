#pragma once

#include "boost/multiprecision/cpp_dec_float.hpp"

using Float = boost::multiprecision::number<boost::multiprecision::cpp_dec_float<1024>>;

inline Float operator*(const Float& a, const Float& b)
{
    Float copy = a;
    copy *= b;
    return copy;
}

inline Float operator/(const Float& a, const Float& b)
{
    Float copy = a;
    copy /= b;
    return copy;
}

inline Float operator-(const Float& a, const Float& b)
{
    Float copy = a;
    copy -= b;
    return copy;
}

inline Float operator+(const Float& a, const Float& b)
{
    Float copy = a;
    copy += b;
    return copy;
}
