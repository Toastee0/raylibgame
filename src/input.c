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

// Include viewportX and cellSize as external variables
extern int viewportX;
extern int viewportY;
extern int cellSize;

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
    
    // Handle viewport panning with arrow keys
    int panSpeed = 10; // Speed of panning in pixels

    // Update viewport panning logic to use separate content offset
    if (IsKeyDown(KEY_RIGHT)) {
        viewportContentOffsetX += panSpeed;
        if (viewportContentOffsetX > GRID_WIDTH * cellSize - gameWidth) {
            viewportContentOffsetX = GRID_WIDTH * cellSize - gameWidth; // Prevent scrolling outside the gamefield
        }
    }
    if (IsKeyDown(KEY_LEFT)) {
        viewportContentOffsetX -= panSpeed;
        if (viewportContentOffsetX < 0) {
            viewportContentOffsetX = 0; // Prevent scrolling outside the gamefield
        }
    }

    // Adjust the calculation for the viewport height to include all visible cells
    int viewportHeight = GetRenderHeight() - 80; // Subtract 80 pixels for UI
    int totalVisibleCells = viewportHeight / cellSize;


    // Lock vertical scrolling
    viewportContentOffsetY = 0;
    
    Vector2 mousePos = GetMousePosition();

    // Correct the calculation for determining if the cursor is in the game area
    bool isInGameArea = mousePos.x > viewportX && mousePos.x < viewportX + gameWidth &&
                        mousePos.y > viewportY && mousePos.y < viewportY + viewportHeight;

    // Correct the calculation for determining if the cursor is over the UI panel
    int uiStartX = GetScreenWidth() - uiPanelWidth; // Correctly calculate the UI panel's starting X position

    // Check if the cursor is over the UI panel
    if (mousePos.x > uiStartX && mousePos.x < GetScreenWidth()) {
        mouseStartedInUI = true;
    } else {
        mouseStartedInUI = false;
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
            // Handle cell placement
            // Adjust mouse position for scrolling offset
            int gridX = (int)((mousePos.x - viewportX + viewportContentOffsetX) / cellSize);
            int gridY = (int)((mousePos.y - viewportY + viewportContentOffsetY) / cellSize);

            // Ensure the grid coordinates are clamped within the viewport bounds
            if (gridX > 0 && gridX < (viewportX + gameWidth) / cellSize &&
                gridY > 0 && gridY < (viewportY + viewportHeight) / cellSize) {
                // Handle cell placement logic here
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                    PlaceCircularPattern(gridX, gridY, currentSelectedType, brushRadius);
                } else if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
                    PlaceCircularPattern(gridX, gridY, CELL_TYPE_AIR, brushRadius);
                }
            }
        }
    }
}
