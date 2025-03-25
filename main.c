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
int brushRadius = 8;  // Default brush radius (in grid cells)
float lastSeedTime = 0;
const float SEED_DELAY = 0.5f;  // Half second delay between seeds
int currentSelectedType = CELL_TYPE_SOIL;  // Default to soil
bool simulationRunning = false;  // Flag to control simulation state
bool simulationPaused = true;    // Start with simulation paused

// Global screen dimensions
const int screenWidth = 1920;
const int screenHeight = 1080;

//----------------------------------------------------------------------------------
// Local Functions Declaration
//----------------------------------------------------------------------------------
static void UpdateDrawFrame(void);

int main(void) {
    InitWindow(screenWidth, screenHeight, "Sandbox");

    // Initialize grid
    InitGrid();

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
    // Always handle input
    HandleInput();
    
    // Only update the grid if simulation is running and not paused
    if (simulationRunning && !simulationPaused) {
        // Update simulation
        UpdateGrid();
    }
    
    BeginDrawing();
        ClearBackground(BLACK);
        DrawGameGrid();
        DrawUI();
        
        // Draw other UI elements
        if (!simulationRunning) {
            DrawRectangle(0, screenHeight/2 - 50, screenWidth, 100, Fade(BLACK, 0.7f));
            DrawText("SET UP INITIAL STATE THEN PRESS SPACE TO START SIMULATION", 
                    screenWidth/2 - MeasureText("SET UP INITIAL STATE THEN PRESS SPACE TO START SIMULATION", 30)/2,
                    screenHeight/2 - 15, 30, WHITE);
        } else if (simulationPaused) {
            DrawRectangle(0, screenHeight/2 - 50, screenWidth, 100, Fade(BLACK, 0.7f));
            DrawText("SIMULATION PAUSED - PRESS SPACE TO RESUME", 
                    screenWidth/2 - MeasureText("SIMULATION PAUSED - PRESS SPACE TO RESUME", 30)/2,
                    screenHeight/2 - 15, 30, WHITE);
        }
    EndDrawing();
}
