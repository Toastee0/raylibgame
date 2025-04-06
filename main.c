/*******************************************************************************************
*
*   raylib [core] example - Sandbox simulation with responsive UI
*
********************************************************************************************/
// code rules: no moisture can be destroyed, only moved, to conseve the total water in the simulation.
//lt branch
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
bool initialStateMessageShown = false; // Flag to track if the initial state message has been shown
bool pauseMessageDrawn = false;       // Flag to track if the pause message has been drawn
bool stateChanged = true;             // Flag to detect state changes

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
    static int lastWidth = 0;
    static int lastHeight = 0;

    // Get DPI scaling factor
    Vector2 dpiScale = GetWindowScaleDPI();

    // Calculate actual pixel dimensions by dividing by DPI scaling factor
    int newWidth = (int)(GetScreenWidth() / dpiScale.x);
    int newHeight = (int)(GetScreenHeight() / dpiScale.y);

    // Only proceed if the window dimensions have changed
    if (newWidth != lastWidth || newHeight != lastHeight) {
        lastWidth = newWidth;
        lastHeight = newHeight;

        // Reserve space for the UI panel
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

        // Ensure the UI panel width remains consistent
        uiPanelWidth = newWidth - gameWidth;

        // Redraw black background after resize
        blackBackgroundDrawn = false;

        TraceLog(LOG_INFO, "Window resized: Cell size adjusted to %d pixels", CELL_SIZE);
    }
}

//----------------------------------------------------------------------------------
// Local Functions Declaration
//----------------------------------------------------------------------------------
static void UpdateDrawFrame(void);
void SetSimulationState(bool running, bool paused);
void HandleStateMessages(void);

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
    HandleInput(); // Handle user input first

    BeginDrawing(); // Start rendering the frame

        ClearBackground(BLACK); // Clear the background at the start of the frame

        if (simulationRunning && !simulationPaused) {
            // Update the simulation state if running
            UpdateGrid();
        }

        DrawGameGrid(); // Render the simulation grid
        DrawUIOnRight(gameHeight, uiPanelWidth); // Render the UI elements

        HandleStateMessages(); // Render state-specific messages on top of everything else

    EndDrawing(); // Finalize and display the frame
}

// Helper function to handle state messages
void HandleStateMessages(void) {
    if (simulationRunning && !simulationPaused) {
        // Clear background once when transitioning to running state
        if (!blackBackgroundDrawn) {
            ClearBackground(BLACK);
            blackBackgroundDrawn = true;
        }
        initialStateMessageShown = true; // Mark initial message as shown
    } else {
        // Blank a bar across the screen under the state text
        DrawRectangle(0, GetScreenHeight() / 2 - 20, gameWidth, 40, BLACK);

        if (!simulationRunning && !initialStateMessageShown) {
            // Draw initial state message
            DrawText("SET UP INITIAL STATE THEN PRESS SPACE TO START SIMULATION", 
                     gameWidth / 2 - MeasureText("SET UP INITIAL STATE THEN PRESS SPACE TO START SIMULATION", 20) / 2,
                     GetScreenHeight() / 2 - 15, 20, WHITE);
        } else if (simulationPaused && !pauseMessageDrawn) {
            // Draw pause message
            DrawText("SIMULATION PAUSED - PRESS SPACE TO RESUME", 
                     gameWidth / 2 - MeasureText("SIMULATION PAUSED - PRESS SPACE TO RESUME", 20) / 2,
                     GetScreenHeight() / 2 - 15, 20, WHITE);
            pauseMessageDrawn = true; // Mark pause message as drawn
        }
    }
}

// Update state change flag when simulation state changes
void SetSimulationState(bool running, bool paused) {
    if (simulationRunning != running || simulationPaused != paused) {
        stateChanged = true;
        pauseMessageDrawn = false; // Reset pause message flag on state change
    }
    simulationRunning = running; // Set simulationRunning to the provided value
    simulationPaused = paused;   // Set simulationPaused to the provided value
}
