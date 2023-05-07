#include <array>
#include <cstddef>
#include <map>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "primes.hpp"
#include "raylib.h"

template <size_t colors_count>
constexpr auto MakePallete()
{
    std::array<Color, colors_count> result{};
    for (Color& c : result)
    {
        c.a = 255;
        c.r = 255;
        c.g = 255;
        c.b = 255;
    }

    auto compute_color = [](double k)
    {
        const auto kk = static_cast<uint8_t>(255.f * k);

        Color c;
        c.a = 255;
        c.r = 255;
        c.g = 255 - kk;
        c.b = 255 - kk;
        return c;
    };

    result.front() = WHITE;
    result.back() = BLACK;

    constexpr size_t gradient_start = 4;
    constexpr size_t gradient_end = 100;

    for (size_t iteration = 1; iteration != colors_count - 1; ++iteration)
    {
        double k = 0.0f;
        if (iteration >= gradient_start)
        {
            if (iteration >= gradient_end)
            {
                k = 1.0f;
            }
            else
            {
                k = static_cast<double>(iteration - gradient_start) / (gradient_end - gradient_start);
            }
        }
        result[iteration] = compute_color(k);
    }

    return result;
}

constexpr size_t DoMandelbrotLoop(double x0, double y0, size_t max_iterations)
{
    double x = 0.0f;
    double y = 0.0f;
    size_t iteration = 0;
    while (x * x + y * y <= 4.0f && iteration != max_iterations)
    {
        double x_temp = x * x - y * y + x0;
        y = 2 * x * y + y0;
        x = x_temp;
        ++iteration;
    }

    return iteration;
}

static constexpr std::string_view kFragmentShader = "";

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
    constexpr size_t max_iterations = 1000;
    constexpr auto pallete = MakePallete<max_iterations + 1>();

    Image image = GenImageColor(screen_width, screen_height, BLANK);
    Texture2D texture = LoadTextureFromImage(image);
    // Shader shader = LoadShaderFromMemory(0, kFragmentShader.data());

    double scale = 1.0f;
    const double scale_factor = 0.95f;

    std::vector<Color> pixels;
    std::array<std::unique_ptr<std::thread>, 8> workers;
    std::array<WorkerData, 8> workers_data{};

    for (size_t i = 0; i != 8; ++i)
    {
        workers[i] = std::make_unique<std::thread>(
            [&, index = i]()
            {
                WorkerData& worker_data = workers_data[index];
                while (true)
                {
                    // Wait for task or exit command
                    while (!worker_data.in_progress)
                    {
                        if (worker_data.must_stop)
                        {
                            return;
                        }
                    }

                    const double x_range = (max_x - min_x) * scale;
                    const double y_range = (max_y - min_y) * scale;
                    const double sx = camera_x - x_range / 2;
                    const double sy = camera_y - y_range / 2;
                    for (auto x = 0; x != screen_width; ++x)
                    {
                        const double px = sx + x_range * static_cast<double>(x) / screen_width;
                        for (auto y = worker_data.begin_pixel_y; y != worker_data.end_pixel_y; ++y)
                        {
                            const double py = sy + y_range * static_cast<double>(y) / screen_height;
                            const size_t iterations = DoMandelbrotLoop(px, py, max_iterations);
                            const Color color = pallete[iterations];
                            pixels[y * screen_width + x] = color;
                        }
                    }

                    worker_data.in_progress = false;
                }
            });
    }

    while (!WindowShouldClose())
    {
        if (IsKeyDown(KEY_E)) scale = std::clamp(scale * scale_factor, 0.000, 1.0);
        if (IsKeyDown(KEY_Q)) scale = std::clamp(scale / scale_factor, 0.000, 1.0);
        const double x_range = (max_x - min_x) * scale;
        const double y_range = (max_y - min_y) * scale;
        if (IsKeyDown(KEY_W)) camera_y -= y_range * 0.1f;
        if (IsKeyDown(KEY_S)) camera_y += y_range * 0.1f;
        if (IsKeyDown(KEY_A)) camera_x -= x_range * 0.1f;
        if (IsKeyDown(KEY_D)) camera_x += x_range * 0.1f;

        pixels.resize(screen_width * screen_height);

        BeginDrawing();
        ClearBackground(WHITE);

        screen_width = GetScreenWidth();
        screen_height = GetScreenHeight();

        const int rows_per_worker = screen_height / static_cast<int>(workers.size());
        for (size_t i = 0; i != workers.size(); ++i)
        {
            auto& worker_data = workers_data[i];
            worker_data.begin_pixel_y = i * rows_per_worker;
            worker_data.end_pixel_y = (i + 1) * rows_per_worker;
            worker_data.in_progress = true;
        }

        for (auto& worker_data : workers_data)
        {
            while (worker_data.in_progress)
                ;
        }

        UpdateTexture(texture, pixels.data());
        DrawTexture(texture, 0, 0, WHITE);
        EndDrawing();
    }

    for (size_t index = 0; index != workers_data.size(); ++index)
    {
        workers_data[index].must_stop = true;
        workers[index]->join();
    }

    // UnloadShader(shader);
    UnloadTexture(texture);
    UnloadImage(image);
    CloseWindow();

    return 0;
}