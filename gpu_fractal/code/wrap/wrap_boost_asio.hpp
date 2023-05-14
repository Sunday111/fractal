#pragma once

#include "klgl/macro/warning_suppress.hpp"

// clang-format off

#if defined(_MSC_VER)
        warning_push \
        _Pragma("warning(disable : 4242)") \
        _Pragma("warning(disable : 5054)")
#endif

#include "boost/asio.hpp"

warning_pop;

// clang-format on
