#include "rendering.h"
#include "raylib.h"
#include "grid.h"
#include "cell_types.h"
#include <stdio.h>

// External variables needed for UI rendering
extern int brushRadius;
extern int currentSelectedType;

// Declare viewportX, viewportY, and cellSize as global variables
int viewportX = 0;
int viewportY = 0;
int cellSize = 8; // Default value, will be updated dynamically in DrawGameGrid

// Update the viewport and cell size dynamically based on the window resolution
void DrawGameGrid(void) {
    // Get the current render dimensions
    int screenWidth = GetRenderWidth();
    int screenHeight = GetRenderHeight();

    // Calculate DPI scaling factor
    Vector2 dpiScale = GetWindowScaleDPI(); // Adjust viewport size based on DPI

    // Use the horizontal DPI scaling factor for calculations
    float dpiScaleFactor = dpiScale.x;

    // Calculate UI scaling and viewport dimensions
    float uiScale = (screenWidth >= 3840 && screenHeight >= 2160) ? 1.5f : 1.0f;
    int uiWidth = 300 * uiScale * dpiScaleFactor; // UI panel width scales with resolution and DPI
    int viewportWidth = screenWidth - uiWidth *2; // Subtract UI width from total screen width
    int viewportHeight = screenHeight; // Use full screen height for viewport

    // Adjust cell size dynamically (8x8 at 1080p fullscreen)
    int baseCellSize = 8;
    float scaleFactor = ((float)viewportHeight / 1080.0f) * dpiScaleFactor;
    cellSize = baseCellSize * scaleFactor;

    // Set the viewport to always start at the top-left corner of the window
    viewportX = 0;
    viewportY = 0;

    // Begin the scissor mode to restrict drawing to the viewport
    BeginScissorMode(viewportX, viewportY, viewportWidth, viewportHeight);

    // Draw only the cells within the viewport
    int startRow = viewportY / cellSize;
    int endRow = (viewportY + viewportHeight) / cellSize;
    int startCol = viewportX / cellSize;
    int endCol = (viewportX + viewportWidth) / cellSize;

    for (int i = startRow; i < endRow; i++) {
        for (int j = startCol; j < endCol; j++) {
            if (i >= 0 && i < GRID_HEIGHT && j >= 0 && j < GRID_WIDTH) {
                Color cellColor = grid[i][j].baseColor;
                DrawRectangle(
                    j * cellSize - viewportX, // No offset for UI width since viewport starts at (0, 0)
                    i * cellSize - viewportY,
                    cellSize - 1,
                    cellSize - 1,
                    cellColor
                );
            }
        }
    }

    // End the scissor mode
    EndScissorMode();
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

    // Adjust cursor position based on viewport transformations
    int adjustedMouseX = (int)((mousePos.x + viewportX) / cellSize) * cellSize;
    int adjustedMouseY = (int)((mousePos.y + viewportY) / cellSize) * cellSize;

    // Draw current brush at adjusted mouse position
    DrawCircleLines(adjustedMouseX, adjustedMouseY, brushRadius * CELL_SIZE, WHITE);
    
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
    // Dynamically calculate the UI panel width based on the intended fixed width of 300 pixels
    int screenWidth = GetScreenWidth();
    Vector2 dpiScale = GetWindowScaleDPI();
    float dpiScaleFactor = dpiScale.x;
    int uiWidth = 300 * dpiScaleFactor; // Scale the fixed width by the DPI factor

    // Ensure the UI width does not exceed a reasonable percentage of the screen width
    if (uiWidth > screenWidth * 0.3) {
        uiWidth = screenWidth * 0.3; // Cap the UI width at 30% of the screen width
    }

    // Get the starting X position for the UI panel (align to the top-right corner)
    int uiStartX = screenWidth - (uiWidth*2); // Correctly position the UI panel based on its width

    // Draw background for UI panel
    DrawRectangle(uiStartX, 0, uiWidth, height, Fade(DARKGRAY, 0.8f));

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
    int buttonsPerRow = (uiWidth - 40) / (buttonSize + padding);
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
    DrawCircleLines(startX + uiWidth/2, controlsY + 50, brushRadius * 3, WHITE);
    
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
