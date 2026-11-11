
#include "raylib.h"

int main(void)
{
    InitWindow(800, 450, "Raylib Test");
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText("Raylib is working!",
                 230, 190, 30, DARKBLUE);

        DrawText("Press ESC to close.",
                 280, 240, 20, GRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
