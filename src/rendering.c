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

// Camera for grid navigation
Camera2D camera = {0};
bool cameraInitialized = false;
Vector2 cameraTarget = {0, 0};

// Declare cellSize as a global variable
int cellSize = 8; // Default value, will be updated dynamically in DrawGameGrid

// Update the viewport and cell size dynamically based on the window resolution
void DrawGameGrid(void) {
    // Get the current render dimensions
    int screenWidth = GetRenderWidth();
    int screenHeight = GetRenderHeight();

    // Calculate UI scaling and viewport dimensions
    int uiWidth = 300; // UI panel width
    int viewportWidth = screenWidth - uiWidth; // Subtract UI width from total screen width
    int viewportHeight = screenHeight; // Use full height of screen

    // Add DPI scaling logic
    Vector2 dpiScale = GetWindowScaleDPI();

    // Initialize camera on first call
    if (!cameraInitialized) {
        // Calculate minimum zoom to fill the viewport
        float minZoomX = (float)viewportWidth / (GRID_WIDTH * CELL_SIZE);
        float minZoomY = (float)viewportHeight / (GRID_HEIGHT * CELL_SIZE);
        float minZoom = (minZoomX < minZoomY) ? minZoomX : minZoomY;
        
        camera.zoom = minZoom;
        
        // Position the camera to show the top-left corner of the grid at the top-left of the viewport
        // To align top-left corner of grid with top-left of viewport:
        camera.target.x = (viewportWidth / (2 * camera.zoom));
        camera.target.y = (viewportHeight / (2 * camera.zoom));
        
        camera.offset = (Vector2){(float)viewportWidth / 2.0f, (float)viewportHeight / 2.0f};
        camera.rotation = 0.0f;
        cameraTarget = camera.target;
        cameraInitialized = true;
        
        TraceLog(LOG_INFO, "Camera initialized - Target: (%f, %f), Offset: (%f, %f), Zoom: %f", 
                 camera.target.x, camera.target.y, camera.offset.x, camera.offset.y, camera.zoom);
    }
    
    // Handle camera movement with arrow keys (pan around the grid)
    float moveSpeed = 10.0f / camera.zoom;
    if (IsKeyDown(KEY_RIGHT)) cameraTarget.x += moveSpeed;
    if (IsKeyDown(KEY_LEFT)) cameraTarget.x -= moveSpeed;
    if (IsKeyDown(KEY_DOWN)) cameraTarget.y += moveSpeed;
    if (IsKeyDown(KEY_UP)) cameraTarget.y -= moveSpeed;
    
    // Smooth camera movement with interpolation
    camera.target.x = camera.target.x * 0.92f + cameraTarget.x * 0.08f;
    camera.target.y = camera.target.y * 0.92f + cameraTarget.y * 0.08f;
    
    // Handle camera zoom with plus/minus keys
    if (IsKeyDown(KEY_EQUAL)) camera.zoom *= 1.02f;
    if (IsKeyDown(KEY_MINUS)) camera.zoom *= 0.98f;
    
    // Handle zoom with mouse wheel when CTRL is held down (changed from non-SHIFT to CTRL)
    float wheel = GetMouseWheelMove();
    if (wheel != 0 && IsKeyDown(KEY_LEFT_CONTROL)) {
        // Zoom with mouse wheel when CTRL is pressed
        Vector2 mousePos = GetMousePosition();
        
        // Only zoom if mouse is in the game area (not UI)
        if (mousePos.x < viewportWidth) {
            // Get world position before zoom
            Vector2 worldBeforeZoom = GetScreenToWorld2D(mousePos, camera);
            
            // Apply zoom
            camera.zoom += wheel * 0.05f;
            
            // Calculate minimum zoom to fill the viewport
            float minZoomX = (float)viewportWidth / (GRID_WIDTH * CELL_SIZE);
            float minZoomY = (float)viewportHeight / (GRID_HEIGHT * CELL_SIZE);
            float minZoom = (minZoomX < minZoomY) ? minZoomX : minZoomY;
            
            // Clamp zoom to avoid issues
            if (camera.zoom < minZoom) camera.zoom = minZoom;
            if (camera.zoom > 5.0f) camera.zoom = 5.0f;
            
            // Get world position after zoom
            Vector2 worldAfterZoom = GetScreenToWorld2D(mousePos, camera);
            
            // Update camera target to zoom toward/from mouse position
            camera.target.x += worldBeforeZoom.x - worldAfterZoom.x;
            camera.target.y += worldBeforeZoom.y - worldAfterZoom.y;
            cameraTarget = camera.target;
        }
    } else {
        // Calculate minimum zoom to fill the viewport
        float minZoomX = (float)viewportWidth / (GRID_WIDTH * CELL_SIZE);
        float minZoomY = (float)viewportHeight / (GRID_HEIGHT * CELL_SIZE);
        float minZoom = (minZoomX < minZoomY) ? minZoomX : minZoomY;
        
        // Apply zoom limits when not using wheel zoom
        if (camera.zoom < minZoom) camera.zoom = minZoom;
        if (camera.zoom > 5.0f) camera.zoom = 5.0f;
    }
    
    // Always update camera offset to match the viewport center
    camera.offset = (Vector2){(float)viewportWidth / 2.0f, (float)viewportHeight / 2.0f};
    
    // Begin scissor mode to constrain rendering to the viewport
    BeginScissorMode(0, 0, viewportWidth, viewportHeight);
    
    // Begin 2D camera mode
    BeginMode2D(camera);
    
    // Fixed cell size for consistent rendering
    int effectiveCellSize = CELL_SIZE;
    
    // Calculate visible area based on camera position and zoom
    float visibleWidthWorld = viewportWidth / camera.zoom;
    float visibleHeightWorld = viewportHeight / camera.zoom;
    
    int startRow = (int)((camera.target.y - visibleHeightWorld/2) / effectiveCellSize);
    int startCol = (int)((camera.target.x - visibleWidthWorld/2) / effectiveCellSize);
    
    // Calculate end coordinates with padding
    int endRow = startRow + (int)(visibleHeightWorld / effectiveCellSize) + 4;  // Extra padding to prevent pop-in
    int endCol = startCol + (int)(visibleWidthWorld / effectiveCellSize) + 4;   // Extra padding to prevent pop-in
    
    // Clamp to grid boundaries
    if (startRow < 0) startRow = 0;
    if (startCol < 0) startCol = 0;
    if (endRow > GRID_HEIGHT) endRow = GRID_HEIGHT;
    if (endCol > GRID_WIDTH) endCol = GRID_WIDTH;
    
    // Draw background for the entire grid
    DrawRectangle(0, 0, GRID_WIDTH * effectiveCellSize, GRID_HEIGHT * effectiveCellSize, 
                  Fade(BLACK, 0.9f));
    
    // Draw only the cells that are within the visible area
    for (int i = startRow; i < endRow; i++) {
        for (int j = startCol; j < endCol; j++) {
            // Safety check to prevent segfaults
            if (i >= 0 && i < GRID_HEIGHT && j >= 0 && j < GRID_WIDTH && grid && grid[i]) {
                Color cellColor = grid[i][j].baseColor;
                DrawRectangle(
                    j * effectiveCellSize,
                    i * effectiveCellSize,
                    effectiveCellSize,
                    effectiveCellSize,
                    cellColor
                );
            }
        }
    }
    
    // Draw grid lines (optional) when zoomed in
    if (camera.zoom > 2.0f) {
        for (int i = startRow; i <= endRow; i++) {
            DrawLine(
                startCol * effectiveCellSize,
                i * effectiveCellSize,
                endCol * effectiveCellSize,
                i * effectiveCellSize,
                Fade(DARKGRAY, 0.3f)
            );
        }
        
        for (int j = startCol; j <= endCol; j++) {
            DrawLine(
                j * effectiveCellSize,
                startRow * effectiveCellSize,
                j * effectiveCellSize,
                endRow * effectiveCellSize,
                Fade(DARKGRAY, 0.3f)
            );
        }
    }
    
    // End 2D camera mode
    EndMode2D();
    
    // End scissor mode
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
    int indicatorRadius = brushRadius; // Remove the scaling factor to show actual size
    int centerX = GetScreenWidth() - margin - indicatorRadius;
    int centerY = margin + indicatorRadius;
    
    // Draw outer circle (border)
    DrawCircleLines(centerX, centerY, indicatorRadius, WHITE);
    
    // Draw inner filled circle with semi-transparency
    DrawCircle(centerX, centerY, indicatorRadius - 2, Fade(DARKGRAY, 0.7f));
    
    // Draw text showing the actual brush radius
    char radiusText[20];
    snprintf(radiusText, sizeof(radiusText), "Size: %d cell(s)", brushRadius*2-1);
    DrawText(radiusText, centerX - 50, centerY - 10, 20, WHITE);
    
    // Draw current brush at mouse position in game area
    Vector2 mousePos = GetMousePosition();
    int uiStartX = GetScreenWidth() - 300; // UI panel width
    
    if (mousePos.x < uiStartX) {
        // Only draw brush preview in game area
        Vector2 worldPos = GetScreenToWorld2D(mousePos, camera);
        Vector2 screenPos = GetWorldToScreen2D(worldPos, camera);
        
        // Draw the brush circle at the correct screen position - show actual size
        DrawCircleLines((int)screenPos.x, (int)screenPos.y, brushRadius * camera.zoom, WHITE);
    }
    
    // Draw other UI elements
    DrawFPS(10, 10);
    
    // Draw UI instructions
    DrawText("Left Click: Place cells", 10, 30, 20, WHITE);
    DrawText("Right Click: Erase (Air)", 10, 50, 20, WHITE);
    DrawText("Middle Click: Place Water", 10, 70, 20, WHITE);
    DrawText("Space: Start/Pause simulation", 10, 90, 20, WHITE);
    DrawText("Arrow Keys: Move camera", 10, 110, 20, WHITE);
    DrawText("+/-: Zoom camera", 10, 130, 20, WHITE);
    DrawText("Shift+Mouse Wheel: Change brush size", 10, 150, 20, WHITE);
    
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
    DrawText("Arrow Keys: Move camera", startX, simControlsY + 80, 18, WHITE);
    DrawText("+/-: Zoom camera", startX, simControlsY + 105, 18, WHITE);
    DrawText("Shift+Wheel: Change brush size", startX, simControlsY + 130, 18, WHITE);

    // Initialize default values for the UI strings
    static char cellMoistureText[50] = "Moisture: N/A";
    static char cellTypeText[50] = "Type: N/A";
    static char cellUnderCursorText[50] = "Cell: N/A";
    static char cellFallingText[50] = "Falling: N/A";  // Add new text field for falling state

    // Show cell information under cursor using camera coordinates
    if (grid != NULL) {
        Vector2 mousePos = GetMousePosition();
        
        // Convert screen position to world position using the camera
        Vector2 worldPos = GetScreenToWorld2D(mousePos, camera);
        
        // Calculate grid cell from world position
        int cellX = (int)(worldPos.x / CELL_SIZE);
        int cellY = (int)(worldPos.y / CELL_SIZE);
        
        // Ensure the cell coordinates are within the grid bounds
        if (cellX >= 0 && cellX < GRID_WIDTH && cellY >= 0 && cellY < GRID_HEIGHT) {
            snprintf(cellUnderCursorText, sizeof(cellUnderCursorText), "Cell: (%d, %d)", cellX, cellY);
            snprintf(cellMoistureText, sizeof(cellMoistureText), "Moisture: %d", grid[cellY][cellX].moisture);
            snprintf(cellFallingText, sizeof(cellFallingText), "Falling: %s", grid[cellY][cellX].is_falling ? "Yes" : "No");
            
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
            snprintf(cellUnderCursorText, sizeof(cellUnderCursorText), "Cell: (%d, %d)", cellX, cellY);
            snprintf(cellMoistureText, sizeof(cellMoistureText), "Moisture: N/A");
            snprintf(cellFallingText, sizeof(cellFallingText), "Falling: N/A");
            snprintf(cellTypeText, sizeof(cellTypeText), "Type: Out of bounds");
        }
    } else {
        // Reset to default values if the grid is not initialized
        snprintf(cellUnderCursorText, sizeof(cellUnderCursorText), "Cell: N/A");
        snprintf(cellMoistureText, sizeof(cellMoistureText), "Moisture: N/A");
        snprintf(cellFallingText, sizeof(cellFallingText), "Falling: N/A");
        snprintf(cellTypeText, sizeof(cellTypeText), "Type: N/A");
    }
    
    // Display cell information
    DrawText(cellUnderCursorText, startX, simControlsY + 155, 18, WHITE);
    DrawText(cellMoistureText, startX, simControlsY + 175, 18, WHITE);
    DrawText(cellFallingText, startX, simControlsY + 195, 18, WHITE);  // Display falling state
    DrawText(cellTypeText, startX, simControlsY + 215, 18, WHITE);  // Move type information down one row

    // Draw performance meter
    DrawFPS(startX, height - 30);
}
