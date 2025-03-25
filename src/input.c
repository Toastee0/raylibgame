#include "input.h"
#include "raylib.h"
#include "grid.h"
#include "cell_actions.h"
#include "cell_types.h"

// External variables needed for input handling
extern int brushRadius;
extern float lastSeedTime;
extern const float SEED_DELAY;
extern int currentSelectedType;  
extern bool simulationRunning;
extern bool simulationPaused;

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
    
    // Handle brush size changes with mouse wheel
    float wheelMove = GetMouseWheelMove();
    if(wheelMove != 0) {
        brushRadius += (int)wheelMove;
        // Clamp brush radius between 1 and 32
        brushRadius = (brushRadius < 1) ? 1 : ((brushRadius > 32) ? 32 : brushRadius);
    }
    
    Vector2 mousePos = GetMousePosition();
    bool isInUIArea = (mousePos.y < 84);  // 64px button + 20px padding
    
    // Track when mouse is initially pressed in the UI area
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        mouseStartedInUI = isInUIArea;
        
        // Handle UI clicks
        if (isInUIArea) {
            const int buttonSize = 64;
            const int padding = 10;
            const int startX = 10;
            
            // Determine which cell type was clicked
            for (int i = 0; i <= CELL_TYPE_MOSS; i++) {
                int posX = startX + (buttonSize + padding) * i;
                if (mousePos.x >= posX && mousePos.x < posX + buttonSize &&
                    mousePos.y >= 10 && mousePos.y < 10 + buttonSize) {
                    currentSelectedType = i;
                    break;  // Found the button, no need to check others
                }
            }
        }
    }
    
    // Reset mouseStartedInUI when mouse is released
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        mouseStartedInUI = false;
    }
    
    // Only allow drawing if mouse didn't start in UI area
    if (!mouseStartedInUI) {
        // Handle cell placement
        int gridX = (int)(mousePos.x / CELL_SIZE);
        int gridY = (int)(mousePos.y / CELL_SIZE);
        
        // Skip if out of bounds
        if (gridX < 0 || gridX >= GRID_WIDTH || gridY < 0 || gridY >= GRID_HEIGHT) {
            return;
        }
        
        // Handle painting with mouse buttons
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            PlaceCircularPattern(gridX, gridY, currentSelectedType, brushRadius);
        } else if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
            // Right click erases (places air)
            PlaceCircularPattern(gridX, gridY, CELL_TYPE_AIR, brushRadius);
        }
    }
}
