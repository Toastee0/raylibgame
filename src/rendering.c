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

// Declare viewportContentOffsetX and viewportContentOffsetY as external variables
extern int viewportContentOffsetX;
extern int viewportContentOffsetY;

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
    int viewportWidth = screenWidth - uiWidth * 2; // Subtract UI width from total screen width

    // Correct the calculation for the viewport height to ensure it matches the visible area
    int viewportHeight = GetRenderHeight() - 80; // Subtract 80 pixels for UI

    // Adjust the endRow calculation to ensure it fits within the viewport
    int endRow = (viewportContentOffsetY + viewportHeight) / cellSize;
    if ((viewportContentOffsetY + viewportHeight) % cellSize != 0) {
        endRow += 1; // Include partially visible rows
    }
    if (endRow > GRID_HEIGHT) {
        endRow = GRID_HEIGHT; // Clamp to grid height
    }

    // Ensure the grid height matches the viewport height
    int totalVisibleCells = viewportHeight / cellSize;
    if (GRID_HEIGHT > totalVisibleCells) {
        GRID_HEIGHT = totalVisibleCells;
    }

    // Adjust rendering logic to use viewportContentOffsetX and viewportContentOffsetY
    int startRow = viewportContentOffsetY / cellSize;

    int startCol = viewportContentOffsetX / cellSize;
    int endCol = (viewportContentOffsetX + viewportWidth) / cellSize;
    if (endCol > GRID_WIDTH) {
        endCol = GRID_WIDTH;
    }

    // Begin the scissor mode to restrict drawing to the viewport
    BeginScissorMode(viewportX, viewportY, viewportWidth, viewportHeight);

    // Draw only the cells within the viewport, adjusted for content offset
    for (int i = startRow; i < endRow; i++) {
        for (int j = startCol; j < endCol; j++) {
            if (i >= 0 && i < GRID_HEIGHT && j >= 0 && j < GRID_WIDTH) {
                Color cellColor = grid[i][j].baseColor;
                DrawRectangle(
                    (j - startCol) * cellSize, // Adjust for content offset
                    (i - startRow) * cellSize,
                    cellSize, // Use dynamic cell size for width
                    cellSize, // Use dynamic cell size for height
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
    snprintf(brushText, sizeof(brushText), "Brush: %d", brushRadius);
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
    snprintf(radiusText, sizeof(radiusText), "R: %d", brushRadius);
    DrawText(radiusText, centerX - 20, centerY - 10, 20, WHITE);
    
    // Draw current brush at mouse position

    // Scale mouse position to account for DPI
    Vector2 dpiScale = GetWindowScaleDPI();

    // Refine the logic to correctly map mouse position to grid cells

    // Calculate the cell under the mouse
    int cellX = (int)((GetMousePosition().x + viewportContentOffsetX) / cellSize);
    int cellY = (int)((GetMousePosition().y + viewportContentOffsetY) / cellSize);

    // Ensure the cell coordinates are clamped within the grid bounds
    cellX = (cellX < 0) ? 0 : (cellX >= GRID_WIDTH ? GRID_WIDTH - 1 : cellX);
    cellY = (cellY < 0) ? 0 : (cellY >= GRID_HEIGHT ? GRID_HEIGHT - 1 : cellY);

    // Correct scaling for mouse position to grid cell mapping
    float cellSizeX = 1615.0f / 230.0f; // Calculate cell size based on given mouse and cell coordinates
    float cellSizeY = 978.0f / 139.0f;

    // Use the average cell size for consistent scaling
    cellSize = (int)((cellSizeX + cellSizeY) / 2.0f);

    // Update the adjusted mouse position for drawing
    int adjustedMouseX = cellX * cellSize + cellSize / 2 + viewportX;
    int adjustedMouseY = cellY * cellSize + cellSize / 2 + viewportY;

    // Draw current brush at adjusted mouse position
    DrawCircleLines(adjustedMouseX, adjustedMouseY, brushRadius * CELL_SIZE, WHITE);
    
    // Draw other UI elements
    DrawFPS(10, 10);
    
    // Draw UI instructions
    DrawText("Left Click: Place plant", 10, 30, 20, WHITE);
    DrawText("Right Click: Place Soil", 10, 50, 20, WHITE);
    DrawText("Middle Click: Place Water", 10, 70, 20, WHITE);
    DrawText("Mouse Wheel: Adjust brush size", 10, 90, 20, WHITE);
    

    
   
    
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
    snprintf(brushText, sizeof(brushText), "Brush Size: %d", brushRadius);
    DrawText(brushText, startX, controlsY, 20, WHITE);
    
    // Draw brush preview
    DrawCircleLines(startX + uiWidth/2, controlsY + 50, brushRadius * 3, WHITE);
    
    // Draw simulation controls
    int simControlsY = controlsY + 100;
    DrawText("Simulation Controls:", startX, simControlsY, 20, WHITE);
    
    DrawText("Space: Start/Pause", startX, simControlsY + 30, 18, WHITE);
    DrawText("Mouse Wheel: Adjust brush", startX, simControlsY + 55, 18, WHITE);
     // Display cursor position and cell grid position in the info panel

    // Draw moisture info
    int moistureY = simControlsY + 90;
    char moistureText[50];
    snprintf(moistureText, sizeof(moistureText), "Total Moisture: %d", CalculateTotalMoisture());
    DrawText(moistureText, startX, moistureY, 18, WHITE);

    // Add current mouse position to the UI panel
    char mousePosText[50];
    snprintf(mousePosText, sizeof(mousePosText), "Mouse: (%.1f, %.1f)", GetMousePosition().x, GetMousePosition().y);
    DrawText(mousePosText, startX, moistureY + 30, 18, WHITE);

    // Initialize default values for the UI strings
    static char cellMoistureText[50] = "Moisture: N/A";
    static char cellTypeText[50] = "Type: N/A";

    // Check if the cursor is over the play area

    // Add the cell under cursor function back to the UI panel
    static char cellUnderCursorText[50] = "Cell: N/A";

    // Adjust the logic to ensure the entire viewport is recognized
    if (grid != NULL) {
        Vector2 mousePos = GetMousePosition();

        // Calculate the cell under the mouse, considering viewport offsets
        int cellX = (int)((mousePos.x - viewportX + viewportContentOffsetX) / cellSize);
        int cellY = (int)((mousePos.y - viewportY + viewportContentOffsetY) / cellSize);

        // Ensure the cell coordinates are clamped within the grid bounds
        if (cellX > 0 && cellX < GRID_WIDTH && cellY > 0 && cellY < GRID_HEIGHT) {
            snprintf(cellUnderCursorText, sizeof(cellUnderCursorText), "Cell: (%d, %d)", cellX, cellY);
            snprintf(cellMoistureText, sizeof(cellMoistureText), "Moisture: %d", grid[cellY][cellX].moisture);
            const char* cellTypeNames[] = {"Air", "Soil", "Water", "Plant", "Rock", "Moss"};
            snprintf(cellTypeText, sizeof(cellTypeText), "Type: %s", cellTypeNames[grid[cellY][cellX].type]);
        } else {
            // Reset to default values if the cell is out of bounds
            snprintf(cellUnderCursorText, sizeof(cellUnderCursorText), "Cell: N/A");
            snprintf(cellMoistureText, sizeof(cellMoistureText), "Moisture: N/A");
            snprintf(cellTypeText, sizeof(cellTypeText), "Type: N/A");
        }
    } else {
        // Reset to default values if the grid is not initialized
        snprintf(cellUnderCursorText, sizeof(cellUnderCursorText), "Cell: N/A");
        snprintf(cellMoistureText, sizeof(cellMoistureText), "Moisture: N/A");
        snprintf(cellTypeText, sizeof(cellTypeText), "Type: N/A");
    }

    // Draw the cell under cursor text
    DrawText(cellUnderCursorText, startX, moistureY + 110, 18, WHITE);

    // Draw the UI strings
    DrawText(cellMoistureText, startX, moistureY + 70, 18, WHITE);
    DrawText(cellTypeText, startX, moistureY + 90, 18, WHITE);
    
    // Draw performance meter
    DrawFPS(startX, height - 30);
}
