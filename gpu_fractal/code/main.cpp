#include <array>
#include <cstddef>
#include <map>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "fmt/format.h"
#include "raylib.h"
#include "shader.hpp"

struct WorkerData
{
    std::atomic_bool in_progress = false;
    std::atomic_bool must_stop = false;
    size_t begin_pixel_y;
    size_t end_pixel_y;
};

int main()
{
    int screen_width = 800;
    int screen_height = 800;

    InitWindow(screen_width, screen_height, "Fractals");
    SetTargetFPS(60);

    const double min_x = -2.0f;
    const double max_x = 0.47f;
    const double min_y = -1.12f;
    const double max_y = 1.12f;
    double camera_x = min_x + (max_x - min_x) / 2;
    double camera_y = min_y + (max_y - min_y) / 2;

    int color_seed = 300;

    Image image = GenImageColor(screen_width, screen_height, BLANK);
    Texture2D texture = LoadTextureFromImage(image);

    std::string shader_text;
    fmt::format_to(std::back_inserter(shader_text), "{}", kFragmentShaderHeader);
    fmt::format_to(std::back_inserter(shader_text), "{}", kFragmentShaderBody);

    Shader shader = LoadShaderFromMemory(0, shader_text.data());

    auto pos_loc = GetShaderLocation(shader, "uCameraPos");
    auto scale_loc = GetShaderLocation(shader, "uScale");
    auto viewport_size_loc = GetShaderLocation(shader, "uViewportSize");
    auto color_seed_loc = GetShaderLocation(shader, "uColorSeed");

    constexpr double scale_factor = 0.95f;
    int scale_i = 0;

    while (!WindowShouldClose())
    {
        if (IsKeyDown(KEY_E)) scale_i += 1;
        if (IsKeyDown(KEY_Q)) scale_i = std::max(scale_i - 1, 0);
        const double scale = std::pow(scale_factor, scale_i);
        const double x_range = (max_x - min_x) * scale;
        const double y_range = (max_y - min_y) * scale;
        if (IsKeyDown(KEY_W)) camera_y += y_range * 0.01f;
        if (IsKeyDown(KEY_S)) camera_y -= y_range * 0.01f;
        if (IsKeyDown(KEY_A)) camera_x -= x_range * 0.01f;
        if (IsKeyDown(KEY_D)) camera_x += x_range * 0.01f;

        BeginDrawing();
        ClearBackground(WHITE);

        screen_width = GetScreenWidth();
        screen_height = GetScreenHeight();

        color_seed = (color_seed + 1) % 500;

        {
            const float camera_pos[]{static_cast<float>(camera_x), static_cast<float>(camera_y)};
            SetShaderValue(shader, pos_loc, camera_pos, SHADER_UNIFORM_VEC2);
            const auto scale_f = static_cast<float>(scale);
            SetShaderValue(shader, scale_loc, &scale_f, SHADER_UNIFORM_FLOAT);
            const float viewport_size[]{static_cast<float>(screen_width), static_cast<float>(screen_height)};
            SetShaderValue(shader, viewport_size_loc, viewport_size, SHADER_UNIFORM_VEC2);
            SetShaderValue(shader, color_seed_loc, &color_seed, SHADER_UNIFORM_INT);
        }

        BeginShaderMode(shader);
        DrawTexture(texture, 0, 0, WHITE);
        EndShaderMode();
        DrawFPS(30, 30);

        EndDrawing();
    }

    UnloadShader(shader);
    UnloadTexture(texture);
    UnloadImage(image);
    CloseWindow();

    return 0;
}