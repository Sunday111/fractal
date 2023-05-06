#include <cstddef>

#include "raylib.h"

int main()
{
    size_t screen_width = 800;
    size_t screen_height = 800;

    InitWindow(static_cast<int>(screen_width), static_cast<int>(screen_height), "Fractals");

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(RAYWHITE);

        for (int x = 0; x != 1; ++x)
        {
            for (int y = 0; y != 1; ++y)
            {
                DrawPixel(x, y, GREEN);
            }
        }

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
