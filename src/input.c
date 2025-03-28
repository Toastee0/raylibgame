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
extern int gameWidth;
extern int uiPanelWidth;

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
    bool isInGameArea = mousePos.x < gameWidth;
    
    // Handle UI interaction
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !isInGameArea) {
        // UI area click handling
        
        // Calculate UI button positions - must match DrawUIOnRight
        int buttonSize = 64;
        int padding = 10;
        int startX = gameWidth + 20;
        int startY = 90;
        
        // Calculate buttons per row based on UI panel width
        int buttonsPerRow = (uiPanelWidth - 40) / (buttonSize + padding);
        if (buttonsPerRow < 1) buttonsPerRow = 1;
        
        // Check each material button
        for (int i = 0; i <= CELL_TYPE_MOSS; i++) {
            int row = i / buttonsPerRow;
            int col = i % buttonsPerRow;
            int posX = startX + col * (buttonSize + padding);
            int posY = startY + row * (buttonSize + padding + 20);
            
            // Check if click is within this button
            if (mousePos.x >= posX && mousePos.x < posX + buttonSize &&
                mousePos.y >= posY && mousePos.y < posY + buttonSize) {
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
}
