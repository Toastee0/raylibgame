#include "input.h"
#include "raylib.h"
#include "grid.h"
#include "cell_actions.h"
#include "cell_types.h"
#include "button_registry.h"
#include "viewport.h"
#include <time.h>   // For time functions
#include <stdio.h>  // For sprintf function

// External variables needed for input handling
extern int brushRadius;
extern float lastSeedTime;
extern const float SEED_DELAY;
extern int currentSelectedType;  
extern bool simulationRunning;
extern bool simulationPaused;
extern int gameWidth;
extern int uiPanelWidth;

// Access to the camera for coordinate conversion
extern Camera2D camera;

// Track if mouse was initially pressed in UI area
static bool mouseStartedInUI = false;

// Handle user input (mouse and keyboard)
void HandleInput(void) {
    // Handle simulation controls (space to start/pause)
    if (IsKeyPressed(KEY_SPACE)) {
        if (!simulationRunning) {
            simulationRunning = true;
            simulationPaused = false;
        } else {
            simulationPaused = !simulationPaused;
        }
    }
    
    // Handle brush size changes with mouse wheel by default (no modifier key needed)
    float wheelMove = GetMouseWheelMove();
    if (wheelMove != 0 && !IsKeyDown(KEY_LEFT_CONTROL)) {
        brushRadius += (int)wheelMove;
        // Clamp brush radius between 1 and 32
        brushRadius = (brushRadius < 1) ? 1 : ((brushRadius > 32) ? 32 : brushRadius);
    }
    
    // Grid save/load keyboard shortcuts
    if (IsKeyPressed(KEY_F5)) {
        // Save grid with timestamp
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char filename[128];
        sprintf(filename, "save_%04d%02d%02d_%02d%02d%02d.grid", 
                t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                t->tm_hour, t->tm_min, t->tm_sec);
        
        // Save to timestamped file
        if (SaveGridToFile(filename)) {
            // Also update the "last save" file for F9 to work properly
            SaveGridToFile("lastsave.grid");
            printf("Saved to %s and lastsave.grid\n", filename);
        }
    }
    
    // Quick save with F6 (just saves to static file)
    if (IsKeyPressed(KEY_F6)) {
        if (SaveGridToFile("lastsave.grid")) {
            printf("Quick saved to lastsave.grid\n");
        } else {
            printf("Failed to save lastsave.grid\n");
        }
    }
    
    // Load last saved grid with F9
    if (IsKeyPressed(KEY_F9)) {
        LoadGridFromFile("lastsave.grid");
    }
    
    // Get mouse position
    Vector2 mousePos = GetMousePosition();
    
    // Get the screen width and calculate viewport/UI boundaries
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Calculate the game area width based on the actual grid dimensions
    // Ensure all grid cells are accessible by calculating viewport based on grid size
    int gameAreaWidth = (GRID_WIDTH * CELL_SIZE) / camera.zoom; // Convert grid size to screen pixels
    
    // Use the larger of calculated game area and default viewport width
    int viewportWidth = screenWidth - uiPanelWidth;
    if (gameAreaWidth > viewportWidth) {
        // If grid requires more space, prioritize grid access
        viewportWidth = gameAreaWidth;
    }
    
    // Debug output to help diagnose boundary issues
    if (IsKeyPressed(KEY_F1)) {
        printf("DEBUG: Grid width=%d cells, Screen width=%d, Game area width=%d, viewport width=%d\n",
               GRID_WIDTH, screenWidth, gameAreaWidth, viewportWidth);
        printf("DEBUG: Mouse at x=%f, Last clickable cell=%d\n", mousePos.x, (int)(mousePos.x / CELL_SIZE));
        printf("DEBUG: Mouse is in %s\n", mousePos.x < viewportWidth ? "GAME AREA" : "UI AREA");
    }
    
    // Determine if the cursor is in the game area
    bool isInGameArea = (mousePos.x < viewportWidth);
    
    // Track when mouse is initially pressed
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        mouseStartedInUI = !isInGameArea;
    }
    
    // Handle UI interaction
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !isInGameArea) {
        // UI area click handling
        // Use the registered button locations to determine clicks
        for (int i = 0; i <= CELL_TYPE_MOSS; i++) {
            if (IsMouseOverButton(i, mousePos.x, mousePos.y)) {
                currentSelectedType = i;
                break;
            }
        }
        return; // Skip game area handling
    }
    
    // Handle game area interaction only if in game area
    if (isInGameArea) {
        // Only allow drawing if mouse didn't start in UI area
        if (!mouseStartedInUI) {
            // Convert screen position to world position using the camera
            Vector2 worldPos = GetScreenToWorld2D(mousePos, camera);
            
            // Calculate grid cell from world position
            int gridX = (int)(worldPos.x / CELL_SIZE);
            int gridY = (int)(worldPos.y / CELL_SIZE);
            
            // Ensure the grid coordinates are within bounds
            if (gridX >= 0 && gridX < GRID_WIDTH && gridY >= 0 && gridY < GRID_HEIGHT) {
                // Handle cell placement logic here
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                    PlaceCircularPattern(gridX, gridY, currentSelectedType, brushRadius);
                } else if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
                    PlaceCircularPattern(gridX, gridY, CELL_TYPE_AIR, brushRadius);
                } else if (IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON)) {
                    // Use middle button for water placement
                    PlaceCircularPattern(gridX, gridY, CELL_TYPE_WATER, brushRadius);
                }
            }
        }
    }
}
