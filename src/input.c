#include "input.h"
#include "grid.h"
#include "raylib.h"
#include <math.h>

// Toggle state for visualization mode
static bool showTemperature = false;

// Get grid access functions
#define GRID(y, x) (GetGrid()[(y) * GRID_WIDTH + (x)])

//----------------------------------------------------------------------------------
// Handle user input
//----------------------------------------------------------------------------------
void HandleInput(void) {
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 mousePos = GetMousePosition();
        int gridX = (int)(mousePos.x / CELL_SIZE);
        int gridY = (int)(mousePos.y / CELL_SIZE);
        
        AddSand(gridX, gridY, 0);  // Changed to 0 radius for single pixel
    }
    
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 mousePos = GetMousePosition();
        int gridX = (int)(mousePos.x / CELL_SIZE);
        int gridY = (int)(mousePos.y / CELL_SIZE);
        
        AddHeat(gridX, gridY, 2);  // Add heat in a 2-pixel radius
    }
    
    // Toggle between pressure and temperature visualization
    if (IsKeyPressed(KEY_T)) {
        showTemperature = !showTemperature;
    }
}

//----------------------------------------------------------------------------------
// Add sand at position with given radius
//----------------------------------------------------------------------------------
void AddSand(int gridX, int gridY, int radius) {
    // Add sand at the specific position
    if (IsValidPosition(gridX, gridY) && GRID(gridY, gridX).material == MATERIAL_AIR) {
        // Get the pressure that will be displaced
        float displacedPressure = GRID(gridY, gridX).pressure;
        float currentTemperature = GRID(gridY, gridX).temperature; // Preserve temperature
        
        // Place the sand
        GRID(gridY, gridX).material = MATERIAL_SAND;
        GRID(gridY, gridX).velocity_x = 0.0f;
        GRID(gridY, gridX).velocity_y = 0.0f;
        GRID(gridY, gridX).pressure = 0.0f; // Sand has no pressure
        GRID(gridY, gridX).temperature = currentTemperature; // Sand keeps ambient temperature
        
        // Distribute displaced pressure to 4 adjacent cells (0.25 each)
        float pressurePerSide = displacedPressure * 0.25f;
        
        // Left
        if (IsValidPosition(gridX - 1, gridY) && GRID(gridY, gridX - 1).material == MATERIAL_AIR) {
            GRID(gridY, gridX - 1).pressure += pressurePerSide;
        }
        // Right
        if (IsValidPosition(gridX + 1, gridY) && GRID(gridY, gridX + 1).material == MATERIAL_AIR) {
            GRID(gridY, gridX + 1).pressure += pressurePerSide;
        }
        // Up
        if (IsValidPosition(gridX, gridY - 1) && GRID(gridY - 1, gridX).material == MATERIAL_AIR) {
            GRID(gridY - 1, gridX).pressure += pressurePerSide;
        }
        // Down
        if (IsValidPosition(gridX, gridY + 1) && GRID(gridY + 1, gridX).material == MATERIAL_AIR) {
            GRID(gridY + 1, gridX).pressure += pressurePerSide;
        }
    }
}

//----------------------------------------------------------------------------------
// Add heat at position with given radius
//----------------------------------------------------------------------------------
void AddHeat(int gridX, int gridY, int radius) {
    const float HEAT_AMOUNT = 50.0f;  // Amount of heat to add (in Celsius)
    
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            int x = gridX + dx;
            int y = gridY + dy;
            
            if (IsValidPosition(x, y)) {
                // Calculate distance for falloff
                float distance = sqrtf((float)(dx * dx + dy * dy));
                if (distance <= (float)radius) {
                    // Heat falloff based on distance
                    float falloff = 1.0f - (distance / (float)radius);
                    float heatToAdd = HEAT_AMOUNT * falloff;
                    
                    // Add heat to the cell
                    GRID(y, x).temperature += heatToAdd;
                    
                    // Clamp temperature to maximum
                    if (GRID(y, x).temperature > 150.0f) {
                        GRID(y, x).temperature = 150.0f;
                    }
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------
// Get current visualization mode
//----------------------------------------------------------------------------------
bool GetShowTemperature(void) {
    return showTemperature;
}
