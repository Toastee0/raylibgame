#include "rendering.h"
#include "grid.h"
#include "input.h"
#include "raylib.h"

// Get grid access functions
#define GRID(y, x) (GetGrid()[(y) * GRID_WIDTH + (x)])

//----------------------------------------------------------------------------------
// Draw the simulation grid
//----------------------------------------------------------------------------------
void DrawSimulationGrid(void) {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            Color cellColor = GetCellColor(GRID(y, x));
            
            DrawRectangle(x * CELL_SIZE, y * CELL_SIZE, 
                         CELL_SIZE, CELL_SIZE, cellColor);
        }
    }
}

//----------------------------------------------------------------------------------
// Draw user interface
//----------------------------------------------------------------------------------
void DrawUI(void) {
    // Draw instructions
    DrawText("Left Click: Add Sand", 10, 10, 20, WHITE);
    DrawText("Right Click: Add Heat", 10, 85, 20, WHITE);
    if (GetShowTemperature()) {
        DrawText("Temperature mode: Blue=Cold, Red=Hot", 10, 35, 20, WHITE);
    } else {
        DrawText("Pressure mode: Blue=High, Red=Low", 10, 35, 20, WHITE);
    }
    DrawText("Press T: Toggle Temperature/Pressure", 10, 60, 20, WHITE);
}

//----------------------------------------------------------------------------------
// Get color for a cell based on material and pressure/temperature
//----------------------------------------------------------------------------------
Color GetCellColor(Cell cell) {
    bool showTemperature = GetShowTemperature();
    
    switch (cell.material) {
        case MATERIAL_SAND:
            if (showTemperature) {
                // Show sand temperature: blue for cold, yellow for normal, red for hot
                // Temperature range: 5°C (cold/blue) to 35°C (hot/red)
                float tempNormalized = (cell.temperature - 5.0f) / 30.0f; // Normalize to 0-1
                if (tempNormalized < 0.0f) tempNormalized = 0.0f;
                if (tempNormalized > 1.0f) tempNormalized = 1.0f;
                
                if (tempNormalized < 0.5f) {
                    // Cold to normal: blue to yellow
                    float coldIntensity = (0.5f - tempNormalized) * 2.0f;
                    return (Color){(int)(255 * (1.0f - coldIntensity)), (int)(255 * (1.0f - coldIntensity)), (int)(255 * coldIntensity), 255};
                } else {
                    // Normal to hot: yellow to red
                    float hotIntensity = (tempNormalized - 0.5f) * 2.0f;
                    return (Color){255, (int)(255 * (1.0f - hotIntensity)), 0, 255};
                }
            } else {
                return YELLOW; // Normal sand color when showing pressure
            }
            
        case MATERIAL_AIR:
        default:
            if (showTemperature) {
                // Temperature visualization for air: blue for cold, red for hot
                // Temperature range: 5°C (cold/blue) to 35°C (hot/red)
                float tempNormalized = (cell.temperature - 5.0f) / 30.0f; // Normalize to 0-1
                if (tempNormalized < 0.0f) tempNormalized = 0.0f;
                if (tempNormalized > 1.0f) tempNormalized = 1.0f;
                
                if (tempNormalized < 0.5f) {
                    // Cold: blue intensity
                    int blueValue = (int)((0.5f - tempNormalized) * 2.0f * 255);
                    return (Color){0, 0, blueValue, 255};
                } else if (tempNormalized > 0.5f) {
                    // Hot: red intensity
                    int redValue = (int)((tempNormalized - 0.5f) * 2.0f * 255);
                    return (Color){redValue, 0, 0, 255};
                } else {
                    // Normal temperature - very dark gray
                    return (Color){10, 10, 10, 255};
                }
            } else {
                // Pressure visualization (original code)
                if (cell.pressure > 1.0f) {
                    // High pressure - blue intensity (1.0 to 9.0 maps to 0 to 255)
                    float intensity = (cell.pressure - 1.0f) / 8.0f;  // Normalize to 0-1
                    if (intensity > 1.0f) intensity = 1.0f;
                    int blueValue = (int)(intensity * 255);
                    return (Color){0, 0, blueValue, 255};
                } else if (cell.pressure < 1.0f) {
                    // Low pressure - red intensity (1.0 to -9.0 maps to 0 to 255)
                    float intensity = (1.0f - cell.pressure) / 10.0f;  // Normalize to 0-1
                    if (intensity > 1.0f) intensity = 1.0f;
                    int redValue = (int)(intensity * 255);
                    return (Color){redValue, 0, 0, 255};
                } else {
                    // Normal pressure - very dark gray (almost black)
                    return (Color){10, 10, 10, 255};
                }
            }
    }
}
