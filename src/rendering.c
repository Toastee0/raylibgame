#include "rendering.h"
#include "raylib.h"
#include "grid.h"
#include "cell_types.h"
#include <stdio.h>

// External variables needed for UI rendering
extern int brushRadius;

// Draw the game grid
void DrawGameGrid(void) {
    for(int i = 0; i < GRID_HEIGHT; i++) {
        for(int j = 0; j < GRID_WIDTH; j++) {
            // Add safety check to prevent undefined colors
            Color cellColor = grid[i][j].baseColor;
            
            // Enforce moisture limits
            grid[i][j].moisture = ClampMoisture(grid[i][j].moisture);
            
            // Fix for pink, purple or undefined colors
            if((cellColor.r > 200 && cellColor.g < 100 && cellColor.b > 200) || 
               (cellColor.r > 200 && cellColor.g < 100 && cellColor.b > 100)) {
                // This is detecting pink/purple-ish colors
                switch(grid[i][j].type) {
                    case CELL_TYPE_AIR: // Air
                        cellColor = BLACK;
                        break;
                    case CELL_TYPE_SOIL: // Soil
                        {
                            // Re-calculate soil color based on proper moisture range
                            float intensityPct = (float)grid[i][j].moisture / 100.0f;
                            cellColor = (Color){
                                127 - (intensityPct * 51),
                                106 - (intensityPct * 43),
                                79 - (intensityPct * 32),
                                255
                            };
                        }
                        break;
                    case CELL_TYPE_WATER: // Water
                        {
                            // Re-calculate water color based on proper moisture range
                            float intensityPct = (float)grid[i][j].moisture / 100.0f;
                            cellColor = (Color){
                                0 + (int)(200 * (1.0f - intensityPct)),
                                120 + (int)(135 * (1.0f - intensityPct)),
                                255,
                                255
                            };
                        }
                        break;
                    case CELL_TYPE_PLANT: // Plant
                        cellColor = GREEN;
                        break;
                    case CELL_TYPE_VAPOR: // Vapor
                        {
                            // Recalculate vapor color using proper threshold
                            if(grid[i][j].moisture < 50) {
                                cellColor = BLACK; // Invisible
                            } else {
                                float intensityPct = (float)(grid[i][j].moisture - 50) / 50.0f;
                                int brightness = 128 + (int)(127 * intensityPct);
                                cellColor = (Color){
                                    brightness, brightness, brightness, 255
                                };
                            }
                        }
                        break;
                    default:
                        cellColor = BLACK; // Default fallback
                        break;
                }
                
                // Update the stored color
                grid[i][j].baseColor = cellColor;
            }
            
            DrawRectangle(
                j * CELL_SIZE,
                i * CELL_SIZE,
                CELL_SIZE - 1,
                CELL_SIZE - 1,
                cellColor
            );
        }
    }
}

// Draw UI elements
void DrawUI(void) {
    // Draw the brush size indicator in top-right corner
    int margin = 20;
    int indicatorRadius = brushRadius * 4; // Scale up for better visibility
    int centerX = GetScreenWidth() - margin - indicatorRadius;
    int centerY = margin + indicatorRadius;
    
    // Draw outer circle (border)
    DrawCircleLines(centerX, centerY, indicatorRadius, WHITE);
    
    // Draw inner filled circle with semi-transparency
    DrawCircle(centerX, centerY, indicatorRadius - 2, Fade(DARKGRAY, 0.7f));
    
    // Draw text showing the actual brush radius
    char radiusText[10];
    sprintf(radiusText, "R: %d", brushRadius);
    DrawText(radiusText, centerX - 20, centerY - 10, 20, WHITE);
    
    // Draw current brush at mouse position
    Vector2 mousePos = GetMousePosition();
    DrawCircleLines((int)mousePos.x, (int)mousePos.y, brushRadius * CELL_SIZE, WHITE);
    
    // Draw other UI elements
    DrawFPS(10, 10);
    
    // Draw UI instructions
    DrawText("Left Click: Place plant", 10, 30, 20, WHITE);
    DrawText("Right Click: Place Soil", 10, 50, 20, WHITE);
    DrawText("Middle Click: Place Water", 10, 70, 20, WHITE);
    DrawText("Mouse Wheel: Adjust brush size", 10, 90, 20, WHITE);
    
    // Display total moisture in the system
    char moistureText[50];
    sprintf(moistureText, "Total Moisture: %d", CalculateTotalMoisture());
    DrawText(moistureText, 10, 110, 20, WHITE);
    
    // Add text at bottom of screen
    DrawText("Tree Growth Simulation", 10, GetScreenHeight() - 30, 20, WHITE);
}
