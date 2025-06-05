/*******************************************************************************************
*
*   Raylib Falling Sand Ecosystem Simulation
*
********************************************************************************************/

#include <stdlib.h>
#include "raylib.h"
#include "src/grid.h"
#include "src/simulation.h"
#include "src/rendering.h"
#include "src/input.h"
#include "src/debug_utils.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

//----------------------------------------------------------------------------------
// Local Variables Definition
//----------------------------------------------------------------------------------
// Window settings - using static instead of const since window can be resized
static int screenWidth = 1280;  // Initial width
static int screenHeight = 720;  // Initial height
const int targetFPS = 60;       // This can still be const

// Global state
static Grid* grid = NULL;
static CellMaterial selectedMaterial = MATERIAL_SAND;
static int brushSize = 3;
static bool paused = false;
static bool showDebug = false;

//----------------------------------------------------------------------------------
// Local Functions Declaration
//----------------------------------------------------------------------------------
static void UpdateDrawFrame(void);  // Update and Draw one frame
static void CleanupResources(void); // Clean up resources

//----------------------------------------------------------------------------------
// Main entry point
//----------------------------------------------------------------------------------
int main(void) {
    // Initialization
    //--------------------------------------------------------------------------------------
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Raylib Falling Sand Ecosystem Simulation");
    SetTargetFPS(targetFPS);
    
    // Initialize grid
    grid = grid_init();
    if (!grid) {
        TraceLog(LOG_ERROR, "Failed to initialize grid!");
        CloseWindow();
        return 1;
    }
    
    // Initialize rendering
    if (!rendering_init(screenWidth, screenHeight)) {
        TraceLog(LOG_ERROR, "Failed to initialize rendering!");
        grid_free(grid);
        CloseWindow();
        return 1;
    }
    
    // Calculate optimal cell size based on window dimensions and grid size
    rendering_calculate_optimal_cell_size(grid);
    
    // Initialize input handling
    if (!input_init()) {
        TraceLog(LOG_ERROR, "Failed to initialize input handling!");
        grid_free(grid);
        rendering_cleanup();
        CloseWindow();
        return 1;
    }
    
    // Initialize simulation logic
    if (!simulation_init()) {
        TraceLog(LOG_ERROR, "Failed to initialize simulation!");
        grid_free(grid);
        rendering_cleanup();
        CloseWindow();
        return 1;
    }
    
    // Add some initial materials
    simulation_add_material(grid, GRID_WIDTH / 2, GRID_HEIGHT / 2, 20, MATERIAL_SAND);
    simulation_add_material(grid, GRID_WIDTH / 2 - 30, GRID_HEIGHT / 2, 15, MATERIAL_WATER);
    
    //--------------------------------------------------------------------------------------

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    // Main game loop
    while (!WindowShouldClose() && 
           input_process(grid, &selectedMaterial, &brushSize, &paused, &showDebug))
    {
        UpdateDrawFrame();
        
        // Debug: Save grid to file every 300 frames (5 seconds at 60 fps)
        if (grid->frame_counter % 300 == 0) {
            save_grid_to_file(grid, "grid_debug.json");
        }
    }
    
    CleanupResources();
#endif

    return 0;
}

// Update and Draw frame function
static void UpdateDrawFrame(void) {
    // Update
    //----------------------------------------------------------------------------------
    // Check if window was resized
    if (IsWindowResized()) {
        // Update our screen size variables with the new window dimensions
        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();
        
        rendering_handle_window_resize();
        // Recalculate the optimal cell size when window is resized
        rendering_calculate_optimal_cell_size(grid);
    }
    
    if (!paused) {
        simulation_update(grid);
    }
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();
    
    ClearBackground(RAYWHITE);
    
    // Draw the simulation grid
    rendering_draw_grid(grid);
    
    // Draw UI elements
    rendering_draw_ui(grid, selectedMaterial, brushSize, paused, showDebug);
    
    EndDrawing();
    //----------------------------------------------------------------------------------
}

// Clean up resources
static void CleanupResources(void) {
    simulation_cleanup();
    rendering_cleanup();
    grid_free(grid);
    CloseWindow();
}

