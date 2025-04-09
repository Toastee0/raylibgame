#include "input.h"
#include "raylib.h"
#include "grid.h"
#include "cell_actions.h"
#include "cell_types.h"
#include "button_registry.h"
#include "viewport.h"

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
    
    // Get mouse position
    Vector2 mousePos = GetMousePosition();

    // Get the actual UI panel width as used in DrawUIOnRight in rendering.c
    int screenWidth = GetScreenWidth();
    int uiWidth = 300; // Fixed UI width (matches the value in DrawUIOnRight)
    
    // Ensure UI doesn't exceed 30% of screen (same logic as in DrawUIOnRight)
    if (uiWidth > screenWidth * 0.3) {
        uiWidth = screenWidth * 0.3;
    }
    
    // Calculate the UI starting position exactly as done in rendering.c
    int uiStartX = screenWidth - uiWidth;

    // Determine if the cursor is in the game area or UI area
    bool isInGameArea = mousePos.x < uiStartX;

    // Check if mouse started in UI panel
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || 
        IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) || 
        IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON)) {
        if (mousePos.x > uiStartX && mousePos.x < GetScreenWidth()) {
            mouseStartedInUI = true;
        } else {
            mouseStartedInUI = false;
        }
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
