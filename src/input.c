#include "input.h"
#include "raylib.h"
#include "grid.h"
#include "cell_actions.h"

// External variables needed for input handling
extern int brushRadius;
extern float lastSeedTime;
extern const float SEED_DELAY;

// Handle user input (mouse and keyboard)
void HandleInput(void) {
    // Handle brush size changes with mouse wheel
    float wheelMove = GetMouseWheelMove();
    if(wheelMove != 0) {
        brushRadius += (int)wheelMove;
        // Clamp brush radius between 1 and 32
        brushRadius = (brushRadius < 1) ? 1 : ((brushRadius > 32) ? 32 : brushRadius);
    }
    
    Vector2 mousePos = GetMousePosition();
    // Convert mouse position to grid coordinates by dividing by CELL_SIZE
    int gridX = (int)(mousePos.x / CELL_SIZE);
    int gridY = (int)(mousePos.y / CELL_SIZE);
    
    if(gridX >= 0 && gridX < GRID_WIDTH && gridY >= 0 && gridY < GRID_HEIGHT) {
        if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            float currentTime = GetTime();
            if(currentTime - lastSeedTime >= SEED_DELAY) {
                PlaceCircularPattern(gridX, gridY, CELL_TYPE_PLANT, brushRadius); // Place plant in circular pattern
                lastSeedTime = currentTime;
            }
        }
        else if(IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            PlaceCircularPattern(gridX, gridY, CELL_TYPE_SOIL, brushRadius); // Place soil in circular pattern
        }
        else if(IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
            PlaceCircularPattern(gridX, gridY, CELL_TYPE_WATER, brushRadius); // Place water in circular pattern
        }
    }
}
