/*******************************************************************************************
*
*   raylib [core] example - Sandbox simulation with responsive UI
*
********************************************************************************************/
// code rules: no moisture can be destroyed, only moved, to conseve the total water in the simulation.

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

// Window and UI dimensions
int windowWidth = 1920+300;
int windowHeight = 1080;
int uiPanelWidth = 300;  // Width of right side UI panel
int gameWidth;           // Will be calculated based on grid dimensions
int gameHeight;          // Will be calculated based on grid dimensions
int minGameWidth = 800;  // Minimum game area width
bool blackBackgroundDrawn = false;  // Flag to track initial drawing

// Function to handle window resizing
void HandleWindowResize(void) {
    int newWidth = GetScreenWidth();
    int newHeight = GetScreenHeight();
    
    // Calculate the actual game area (excluding UI panel)
    int actualGameWidth = newWidth - uiPanelWidth;
    
    if (actualGameWidth < minGameWidth) {
        actualGameWidth = minGameWidth;
    }
    
    // Calculate new cell size based on game area and fixed grid dimensions
    int newCellSizeWidth = actualGameWidth / GRID_WIDTH;
    int newCellSizeHeight = newHeight / GRID_HEIGHT;
    
    // Use the smaller dimension to ensure grid fits
    int newCellSize = (newCellSizeWidth < newCellSizeHeight) ? 
                      newCellSizeWidth : newCellSizeHeight;
    
    // Ensure cell size is at least 2 pixels
    if (newCellSize < 2) newCellSize = 2;
    
    // Update the global cell size
    CELL_SIZE = newCellSize;
    
    // Update game area dimensions based on actual grid size
    gameWidth = CELL_SIZE * GRID_WIDTH;
    gameHeight = CELL_SIZE * GRID_HEIGHT;
    
    // Redraw black background after resize
    blackBackgroundDrawn = false;
    
    TraceLog(LOG_INFO, "Window resized: Cell size adjusted to %d pixels", CELL_SIZE);
}

//----------------------------------------------------------------------------------
// Local Functions Declaration
//----------------------------------------------------------------------------------
static void UpdateDrawFrame(void);

int main(void) {
    // Initialize window with resizable flag
    InitWindow(windowWidth, windowHeight, "Sandbox Simulation");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    
    // Initialize grid
    InitGrid();
    
    // Set initial game dimensions
    gameWidth = CELL_SIZE * GRID_WIDTH;
    gameHeight = CELL_SIZE * GRID_HEIGHT;
    
    SetTargetFPS(60);
    
    while (!WindowShouldClose()) {
        // Check if window was resized
        if (IsWindowResized()) {
            HandleWindowResize();
        }
        
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
        // Clear only once when resized
        if (!blackBackgroundDrawn) {
            ClearBackground(BLACK);
            blackBackgroundDrawn = true;
        }
        
        DrawGameGrid();
        DrawUIOnRight(gameHeight, uiPanelWidth);
        
        // Draw other UI elements
        if (!simulationRunning) {
            DrawRectangle(0, GetScreenHeight()/2 - 50, gameWidth, 100, Fade(BLACK, 0.7f));
            DrawText("SET UP INITIAL STATE THEN PRESS SPACE TO START SIMULATION", 
                    gameWidth/2 - MeasureText("SET UP INITIAL STATE THEN PRESS SPACE TO START SIMULATION", 20)/2,
                    GetScreenHeight()/2 - 15, 20, WHITE);
        } else if (simulationPaused) {
            DrawRectangle(0, GetScreenHeight()/2 - 50, gameWidth, 100, Fade(BLACK, 0.7f));
            DrawText("SIMULATION PAUSED - PRESS SPACE TO RESUME", 
                    gameWidth/2 - MeasureText("SIMULATION PAUSED - PRESS SPACE TO RESUME", 20)/2,
                    GetScreenHeight()/2 - 15, 20, WHITE);
        }
    EndDrawing();
}
