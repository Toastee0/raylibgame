/*******************************************************************************************
*
*   Falling Sand Simulation with Air Pressure Visualization - Modular Version
*
********************************************************************************************/

#include "raylib.h"
#include "src/grid.h"
#include "src/simulation.h"
#include "src/input.h"
#include "src/rendering.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

//----------------------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------------------
const int screenWidth = 1200;
const int screenHeight = 800;

//----------------------------------------------------------------------------------
// Function Declarations
//----------------------------------------------------------------------------------
static void UpdateDrawFrame(void);

//----------------------------------------------------------------------------------
// Main entry point
//----------------------------------------------------------------------------------
int main(void) {
    // Initialization
    //--------------------------------------------------------------------------------------
    InitWindow(screenWidth, screenHeight, "Falling Sand with Air Pressure");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    
    InitializeGrid();
    
    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else    
    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        UpdateDrawFrame();
    }
#endif

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//----------------------------------------------------------------------------------
// Update and Draw frame function
//----------------------------------------------------------------------------------
static void UpdateDrawFrame(void) {
    // Update
    //----------------------------------------------------------------------------------
    HandleInput();
    UpdateSimulation();
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

        ClearBackground(BLACK);
        
        DrawSimulationGrid();
        DrawUI();

    EndDrawing();
    //----------------------------------------------------------------------------------
}
