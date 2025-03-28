#include "rendering.h"
#include "raylib.h"
#include "grid.h"
#include "cell_types.h"
#include <stdio.h>

// External variables needed for UI rendering
extern int brushRadius;
extern int currentSelectedType;

// Draw the game grid
void DrawGameGrid(void) {
    for(int i = 0; i < GRID_HEIGHT; i++) {
        for(int j = 0; j < GRID_WIDTH; j++) {
            // Add safety check to prevent undefined colors
            Color cellColor = grid[i][j].baseColor;
            
            // Special case for air - only visible at high moisture levels
            if(grid[i][j].type == CELL_TYPE_AIR) {
                // Air with less than 75% moisture is invisible (black)
                if(grid[i][j].moisture < 75) {
                    cellColor = BLACK;
                } else {
                    // Above 75% moisture, air becomes increasingly visible/white
                    // Calculate a brightness based on moisture (75-100 mapped to 0-255)
                    int brightness = (grid[i][j].moisture - 75) * (255 / 25);
                    cellColor = (Color){brightness, brightness, brightness, 255};
                }
                
                // Update the stored color
                grid[i][j].baseColor = cellColor;
            }
            // Fix for pink, purple or undefined colors for other cell types
            else if((cellColor.r > 200 && cellColor.g < 100 && cellColor.b > 200) || 
                    (cellColor.r > 200 && cellColor.g < 100 && cellColor.b > 100)) {
                // This is detecting pink/purple-ish colors
                switch(grid[i][j].type) {
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
    // Draw cell type selection UI
    const int buttonSize = 64;
    const int padding = 10;
    const int startX = 10;
    const int startY = 10;
    
    // Cell type labels for UI
    const char* typeLabels[] = {
        "Air", "Soil", "Water", "Plant", "Rock", "Moss"
    };
    
    // Cell type colors for UI
    Color typeColors[] = {
        WHITE,                          // Air
        (Color){127, 106, 79, 255},     // Soil
        BLUE,                           // Water
        GREEN,                          // Plant 
        DARKGRAY,                       // Rock
        DARKGREEN                       // Moss
    };
    
    // Draw cell type buttons
    for (int i = 0; i <= CELL_TYPE_MOSS; i++) {
        int posX = startX + (buttonSize + padding) * i;
        
        // Draw button background (highlight if selected)
        DrawRectangle(posX, startY, buttonSize, buttonSize, 
                     (i == currentSelectedType) ? LIGHTGRAY : DARKGRAY);
        
        // Draw cell type color preview
        DrawRectangle(posX + 5, startY + 5, buttonSize - 10, buttonSize - 25, typeColors[i]);
        
        // Draw type name
        DrawText(typeLabels[i], posX + 5, startY + buttonSize - 18, 16, WHITE);
    }
    
    // Draw brush size indicator
    char brushText[32];
    sprintf(brushText, "Brush: %d", brushRadius);
    DrawText(brushText, startX, startY + buttonSize + 10, 20, WHITE);
    
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

// Draw UI panel on the right side of the game area
void DrawUIOnRight(int height, int width) {
    // Get the starting X position for the UI panel (right after the game area)
    extern int gameWidth;
    int uiStartX = gameWidth;
    
    // Draw background for UI panel
    DrawRectangle(uiStartX, 0, width, height, Fade(DARKGRAY, 0.8f));
    
    // Draw title
    DrawText("Sandbox Controls", uiStartX + 20, 20, 24, WHITE);
    
    // Cell type selection UI
    DrawText("Materials:", uiStartX + 20, 60, 20, WHITE);
    
    const int buttonSize = 64;
    const int padding = 10;
    const int startX = uiStartX + 20;
    const int startY = 90;
    
    // Cell type labels for UI
    const char* typeLabels[] = {
        "Air", "Soil", "Water", "Plant", "Rock", "Moss"
    };
    
    // Cell type colors for UI
    Color typeColors[] = {
        WHITE,                          // Air
        (Color){127, 106, 79, 255},     // Soil
        BLUE,                           // Water
        GREEN,                          // Plant 
        DARKGRAY,                       // Rock
        DARKGREEN                       // Moss
    };
    
    // Calculate buttons per row based on UI panel width
    int buttonsPerRow = (width - 40) / (buttonSize + padding);
    if (buttonsPerRow < 1) buttonsPerRow = 1;
    
    // Draw cell type buttons
    for (int i = 0; i <= CELL_TYPE_MOSS; i++) {
        int row = i / buttonsPerRow;
        int col = i % buttonsPerRow;
        int posX = startX + col * (buttonSize + padding);
        int posY = startY + row * (buttonSize + padding + 20);
        
        // Draw button background (highlight if selected)
        DrawRectangle(posX, posY, buttonSize, buttonSize, 
                     (i == currentSelectedType) ? LIGHTGRAY : DARKGRAY);
        
        // Draw cell type color preview
        DrawRectangle(posX + 5, posY + 5, buttonSize - 10, buttonSize - 25, typeColors[i]);
        
        // Draw type name
        DrawText(typeLabels[i], posX + 5, posY + buttonSize - 18, 16, WHITE);
    }
    
    // Draw brush size controls
    int controlsY = startY + ((CELL_TYPE_MOSS+1) / buttonsPerRow + 1) * (buttonSize + padding + 20);
    
    // Draw brush size text
    char brushText[32];
    sprintf(brushText, "Brush Size: %d", brushRadius);
    DrawText(brushText, startX, controlsY, 20, WHITE);
    
    // Draw brush preview
    DrawCircleLines(startX + width/2, controlsY + 50, brushRadius * 3, WHITE);
    
    // Draw simulation controls
    int simControlsY = controlsY + 100;
    DrawText("Simulation Controls:", startX, simControlsY, 20, WHITE);
    
    DrawText("Space: Start/Pause", startX, simControlsY + 30, 18, WHITE);
    DrawText("Mouse Wheel: Adjust brush", startX, simControlsY + 55, 18, WHITE);
    
    // Draw moisture info
    int moistureY = simControlsY + 90;
    char moistureText[50];
    sprintf(moistureText, "Total Moisture: %d", CalculateTotalMoisture());
    DrawText(moistureText, startX, moistureY, 18, WHITE);
    
    // Draw performance meter
    DrawFPS(startX, height - 30);
}
