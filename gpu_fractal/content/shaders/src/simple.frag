uniform vec2 uCameraPos;
uniform vec2 uViewportSize;
uniform float uScale;
uniform vec3 uColorSeed;
out vec4 fragColor;

#define MAX_ITERATIONS 1000

#define ChannelValue(channel)                                                                          \
    float ChannelValue##channel(int iteration){                                                        \
        int offset = MAX_ITERATIONS / 3;                                                         \
        int center = (offset * (2 * channel + 3) / 2);                                           \
        float seeded = mod(float(center) + uColorSeed[channel] * MAX_ITERATIONS, MAX_ITERATIONS);        \
        float rel_dist = (float(iteration) - seeded) / (MAX_ITERATIONS - center);                \
        return abs(rel_dist);                                                                          \
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