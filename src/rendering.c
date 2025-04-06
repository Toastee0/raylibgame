#include "rendering.h"
#include "raylib.h"
#include "grid.h"
#include "cell_types.h"
#include <stdio.h>
#include "button_registry.h"
#include "viewport.h"

// External variables needed for UI rendering
extern int brushRadius;
extern int currentSelectedType;

// Declare cellSize as a global variable
int cellSize = 8; // Default value, will be updated dynamically in DrawGameGrid

// Declare viewportContentOffsetX and viewportContentOffsetY as external variables
extern int viewportContentOffsetX;
extern int viewportContentOffsetY;

// Update the viewport and cell size dynamically based on the window resolution
void DrawGameGrid(void) {
    // Get the current render dimensions
    int screenWidth = GetRenderWidth();

    // Calculate UI scaling and viewport dimensions
    int uiWidth = 300; // UI panel width
    int viewportWidth = screenWidth - uiWidth * 2; // Subtract UI width from total screen width

    // Correct the calculation for the viewport height to ensure it matches the visible area
    int viewportHeight = GetRenderHeight() - 80; // Subtract 80 pixels for UI

    // Add DPI scaling logic
    Vector2 dpiScale = GetWindowScaleDPI();

    // Adjust viewport dimensions and cell size based on DPI scaling
    int adjustedViewportWidth = (int)(viewportWidth / dpiScale.x);
    int adjustedViewportHeight = (int)(viewportHeight / dpiScale.y);
    
    // Ensure cellSize is never zero
    int effectiveCellSize = 8; // Use a fixed cell size instead of scaling it
    
    // Calculate visible cells based on viewport dimensions
    int visibleCols = adjustedViewportWidth / effectiveCellSize;
    int visibleRows = adjustedViewportHeight / effectiveCellSize;
    
    // Calculate starting and ending grid coordinates to display
    int startRow = 0;
    int startCol = 0;
    
    // Ensure we don't go beyond grid boundaries
    int endRow = startRow + visibleRows;
    if (endRow > GRID_HEIGHT) {
        endRow = GRID_HEIGHT;
    }
    
    int endCol = startCol + visibleCols;
    if (endCol > GRID_WIDTH) {
        endCol = GRID_WIDTH;
    }
    
    // Calculate viewport position
    int viewportX = (int)(uiWidth / dpiScale.x);
    int viewportY = 40; // Fixed offset from top

    // Begin the scissor mode to restrict drawing to the viewport
    BeginScissorMode(viewportX, viewportY, adjustedViewportWidth, adjustedViewportHeight);

    // Draw only the cells within the viewport, adjusted for content offset
    for (int i = startRow; i < endRow; i++) {
        for (int j = startCol; j < endCol; j++) {
            // Correct the bounds check to use >= instead of > to avoid segmentation faults
            if (i >= 0 && i < GRID_HEIGHT && j >= 0 && j < GRID_WIDTH && grid && grid[i]) {
                Color cellColor = grid[i][j].baseColor;
                DrawRectangle(
                    (j - startCol) * effectiveCellSize, // Adjust for content offset
                    (i - startRow) * effectiveCellSize,
                    effectiveCellSize, // Use fixed cell size for width
                    effectiveCellSize, // Use fixed cell size for height
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
    
    // Add DPI scaling logic
    Vector2 dpiScale = GetWindowScaleDPI();

    // Use `dpiScale` to adjust the size of UI elements dynamically
    int adjustedButtonSize = (int)(buttonSize / dpiScale.x);
    int adjustedPadding = (int)(padding / dpiScale.x);
    
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
        int posX = startX + (adjustedButtonSize + adjustedPadding) * i;
        
        // Draw button background (highlight if selected)
        DrawRectangle(posX, startY, adjustedButtonSize, adjustedButtonSize, 
                     (i == currentSelectedType) ? LIGHTGRAY : DARKGRAY);
        
        // Draw cell type color preview
        DrawRectangle(posX + 5, startY + 5, adjustedButtonSize - 10, adjustedButtonSize - 25, typeColors[i]);
        
        // Draw type name
        DrawText(typeLabels[i], posX + 5, startY + adjustedButtonSize - 18, 16, WHITE);
    }
    
    // Draw brush size indicator
    char brushText[32];
    snprintf(brushText, sizeof(brushText), "Brush: %d", brushRadius);
    DrawText(brushText, startX, startY + adjustedButtonSize + 10, 20, WHITE);
    
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

    // Refine the logic to correctly map mouse position to grid cells

    // Calculate the cell under the mouse
    int cellX = (int)((GetMousePosition().x + viewportContentOffsetX) / cellSize);
    int cellY = (int)((GetMousePosition().y + viewportContentOffsetY) / cellSize);

    // Ensure the cell coordinates are clamped within the grid bounds (must use > check not >= here or we segmentation fault)
    cellX = (cellX < 0) ? 0 : (cellX > GRID_WIDTH ? GRID_WIDTH - 1 : cellX);
    cellY = (cellY < 0) ? 0 : (cellY > GRID_HEIGHT ? GRID_HEIGHT - 1 : cellY);

    // Correct scaling for mouse position to grid cell mapping
    float cellSizeX = 1615.0f / 230.0f; // Calculate cell size based on given mouse and cell coordinates
    float cellSizeY = 978.0f / 139.0f;

    // Use the average cell size for consistent scaling
    cellSize = (int)((cellSizeX + cellSizeY) / 2.0f);

    // Update the adjusted mouse position for drawing
    int adjustedMouseX = cellX * cellSize + cellSize / 2 + viewportX;
    int adjustedMouseY = cellY * cellSize + cellSize / 2 + viewportY;

    // Ensure `dpiScale` is used in all calculations
    // Adjust mouse position scaling for DPI
    Vector2 mousePos = GetMousePosition();
    mousePos.x /= dpiScale.x;
    mousePos.y /= dpiScale.y;

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

// Adjust the UI panel to always render its top-right corner at the top-right corner of the window
void DrawUIOnRight(int height, int width) {
    // Dynamically calculate the UI panel width based on the intended fixed width of 300 pixels
    int screenWidth = GetScreenWidth();
    int uiWidth = 300; // Fixed width

    // Add DPI scaling logic
    Vector2 dpiScale = GetWindowScaleDPI();

    // Ensure the UI width does not exceed a reasonable percentage of the screen width
    if (uiWidth > screenWidth * 0.3) {
        uiWidth = screenWidth * 0.3; // Cap the UI width at 30% of the screen width
    }

    // Get the starting X position for the UI panel (align to the top-right corner)
    int uiStartX = screenWidth - uiWidth; // Correctly position the UI panel based on its width

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
    
    // Use `dpiScale` to adjust the size of UI elements dynamically
    int adjustedButtonSize = (int)(buttonSize / dpiScale.x);
    int adjustedPadding = (int)(padding / dpiScale.x);
    
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
    
    // Calculate buttons per row based on a fixed 3-column layout
    int buttonsPerRow = 3;
    
    // Clear previous button registrations
    ClearButtonRegistry();

    // Draw cell type buttons
    for (int i = 0; i <= CELL_TYPE_MOSS; i++) {
        int row = i / buttonsPerRow;
        int col = i % buttonsPerRow;
        int posX = startX + col * (adjustedButtonSize + adjustedPadding);
        int posY = startY + row * (adjustedButtonSize + adjustedPadding + 20);
        
        // Draw button background (highlight if selected)
        DrawRectangle(posX, posY, adjustedButtonSize, adjustedButtonSize, 
                     (i == currentSelectedType) ? LIGHTGRAY : DARKGRAY);
        
        // Draw cell type color preview
        DrawRectangle(posX + 5, posY + 5, adjustedButtonSize - 10, adjustedButtonSize - 25, typeColors[i]);
        
        // Draw type name
        DrawText(typeLabels[i], posX + 5, posY + adjustedButtonSize - 18, 16, WHITE);

        // Inform the input system of the button's location
        RegisterButtonLocation(i, posX, posY, adjustedButtonSize, adjustedButtonSize);
    }
    
    // Draw brush size controls
    int controlsY = startY + ((CELL_TYPE_MOSS+1) / buttonsPerRow + 1) * (adjustedButtonSize + adjustedPadding + 20);
    
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

 

    // Add current mouse position to the UI panel
   

    // Initialize default values for the UI strings
    static char cellMoistureText[50] = "Moisture: N/A";
    static char cellTypeText[50] = "Type: N/A";

    // Check if the cursor is over the play area

    // Add the cell under cursor function back to the UI panel
    static char cellUnderCursorText[50] = "Cell: N/A";

    // Adjust the logic to ensure the entire viewport is recognized
    if (grid != NULL) {
        Vector2 mousePos = GetMousePosition();

        // Ensure `dpiScale` is used in all calculations
        // Adjust mouse position scaling for DPI
        mousePos.x /= dpiScale.x;
        mousePos.y /= dpiScale.y;

        // Calculate the cell under the mouse, considering viewport offsets
        int cellX = (int)((mousePos.x - viewportX + viewportContentOffsetX) / cellSize);
        int cellY = (int)((mousePos.y - viewportY + viewportContentOffsetY) / cellSize);

        // Fix the invalid border check:

        // Ensure the cell coordinates are clamped within the grid bounds and handle border cells
        if (cellX >= 0 && cellX < GRID_WIDTH && cellY >= 0 && cellY < GRID_HEIGHT) {
            snprintf(cellUnderCursorText, sizeof(cellUnderCursorText), "Cell: (%d, %d)", cellX, cellY);
            snprintf(cellMoistureText, sizeof(cellMoistureText), "Moisture: %d", grid[cellY][cellX].moisture);
            
            int cellType = grid[cellY][cellX].type;
            if (cellType == CELL_TYPE_BORDER) {
                snprintf(cellTypeText, sizeof(cellTypeText), "Type: Border");
            } else if (cellType >= 0 && cellType <= CELL_TYPE_MOSS) {
                const char* cellTypeNames[] = {"Air", "Soil", "Water", "Plant", "Rock", "Moss"};
                snprintf(cellTypeText, sizeof(cellTypeText), "Type: %s", cellTypeNames[cellType]);
            } else {
                snprintf(cellTypeText, sizeof(cellTypeText), "Type: Unknown (%d)", cellType);
            }
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



   
    // Draw performance meter
    DrawFPS(startX, height - 30);
}
