/*******************************************************************************************
*
*   raylib [core] example - Sandbox simulation
*
*   Welcome to raylib!
*
*   To compile example, just press F5.
*   Note that compiled executable is placed in the same folder as .c file
*
*   You can find all basic examples on C:\raylib\raylib\examples folder or
*   raylib official webpage: www.raylib.com
*
*   Enjoy using raylib. :)
*
*   This example has been created using raylib 1.0 (www.raylib.com)
*   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
*
*   Copyright (c) 2013-2024 Ramon Santamaria (@raysan5)
*
********************************************************************************************/
//code rules: no moisture can be destroyed, only moved, to conseve the total water in the simulation.

#include "raylib.h"
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

// Include our custom headers
#include "src/cell_types.h"
#include "src/grid.h"
#include "src/cell_actions.h"
#include "src/simulation.h"
#include "src/input.h"
#include "src/rendering.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

//----------------------------------------------------------------------------------
// Local Variables Definition (local to this module)
//----------------------------------------------------------------------------------
Camera camera = { 0 };
int brushRadius = 1;  // Default brush radius (in grid cells)
float lastSeedTime = 0;
const float SEED_DELAY = 0.5f;  // Half second delay between seeds

//----------------------------------------------------------------------------------
// Local Functions Declaration
//----------------------------------------------------------------------------------
static void UpdateDrawFrame(void);

//----------------------------------------------------------------------------------
// Main function
//----------------------------------------------------------------------------------
int main() {
    const int screenWidth = 1920;
    const int screenHeight = 1080;

    InitWindow(screenWidth, screenHeight, "Sandbox");

    // Initialize grid
    InitGrid();

    camera.position = (Vector3){ 10.0f, 10.0f, 8.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        UpdateDrawFrame();
    }

    // Cleanup
    CleanupGrid();
    CloseWindow();
    
    return 0;
}

// Update and draw frame function
static void UpdateDrawFrame(void) {
    HandleInput();
    UpdateGrid();
    
    BeginDrawing();
        ClearBackground(BLACK);
        DrawGameGrid();
        DrawUI();
    EndDrawing();
}
