#pragma once

#include <array>
#include <string_view>

#include "lib_fractal/color.hpp"
#include "lib_fractal/pallete.hpp"

static constexpr std::string_view kFragmentShaderHeader = R"(
#version 440

uniform vec2 uCameraPos;
uniform vec2 uViewportSize;
uniform float uScale;
uniform int uColorSeed;
out vec4 fragColor;

#define MAX_ITERATIONS 1000

#define ChannelValue(channel)                                                                 \
    float ChannelValue##channel(int iteration){                                               \
        const int offset = MAX_ITERATIONS / 3;                                                \
        const int center = uColorSeed + (offset * (2 * channel + 3) / 2) % MAX_ITERATIONS;    \
        const float rel_dist = float(iteration - center) / (MAX_ITERATIONS - center);         \
        return abs(rel_dist);                                                                 \
    }

ChannelValue(0)
ChannelValue(1)
ChannelValue(2)

vec3 ColorForIteration(int iteration)
{
    if (iteration == MAX_ITERATIONS)
        return vec3(0, 0, 0);

    return vec3(ChannelValue0(iteration), ChannelValue1(iteration), ChannelValue2(iteration));
}
)";

static constexpr std::string_view kFragmentShaderBody = R"(
int DoMandelbrotLoop(vec2 p0)
{
    vec2 p = vec2(0, 0);
    int iteration = 0;
    while (dot(p, p) <= 4.0 && iteration != MAX_ITERATIONS)
    {
        float x_temp = p.x * p.x - p.y * p.y + p0.x;
        p.y = 2 * p.x * p.y + p0.y;
        p.x = x_temp;
        ++iteration;
    }
    return iteration;
}

void main()
{
    vec2 min_coord = vec2(-2.0, -1.12);
    vec2 max_coord = vec2(0.47, 1.12);
    vec2 range = uScale * (max_coord - min_coord);
    vec2 frag_pos = gl_FragCoord.xy / uViewportSize;
    vec2 coord = uCameraPos + range * (frag_pos - 0.5);
    int iterations = DoMandelbrotLoop(coord);
    fragColor.a = 1;
    fragColor.rgb = ColorForIteration(iterations);
}
)";

namespace glsl
{
template <size_t N>
struct Vec
{
    std::array<float, N> data;
};

}  // namespace glsl

template <size_t N>
struct fmt::formatter<glsl::Vec<N>>
{
    constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin())
    {
        return ctx.end();
    }

    auto format(const glsl::Vec<N>& v, fmt::format_context& ctx) const
    {
        auto out = ctx.out();
        fmt::format_to(out, "vec{}({})", N, fmt::join(v.data, ", "));
        return out;
    }
};
