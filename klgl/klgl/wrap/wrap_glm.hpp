#pragma once

#include "klgl/macro/warning_suppress.hpp"

// clang-format off
#ifdef __clang__
    #define include_glm_begin \
        warning_push \
        _Pragma("GCC diagnostic ignored \"-Wsign-conversion\"") \
        _Pragma("GCC diagnostic ignored \"-Wconversion\"") \
        _Pragma("GCC diagnostic ignored \"-Wfloat-equal\"") \
        _Pragma("GCC diagnostic ignored \"-Wgnu-anonymous-struct\"") \
        _Pragma("GCC diagnostic ignored \"-Wnested-anon-types\"") \
        static_assert(true, "")
#elif defined(__GNUC__) || defined(__GNUG__)
    #define include_glm_begin \
        warning_push \
        _Pragma("GCC diagnostic ignored \"-Wsign-conversion\"") \
        _Pragma("GCC diagnostic ignored \"-Wconversion\"") \
        _Pragma("GCC diagnostic ignored \"-Wduplicated-branches\"") \
        static_assert(true, "")
#elif defined(_MSC_VER)
    #define include_glm_begin \
        warning_push \
        _Pragma("warning(disable : 4201)") \
        static_assert(true, "")
#endif

#define include_glm_end warning_pop static_assert(true, "")

// clang-format on

include_glm_begin;
#include "glm/glm.hpp"
#include "glm/gtc/matrix_access.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
include_glm_end;
