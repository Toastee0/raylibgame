#include "grid.h"
#include <string.h>
#include <stdbool.h>

// Global grid variables
static Cell grid[75][120];  // Using literal values since we can't use const in array declaration
static Cell nextGrid[75][120];

//----------------------------------------------------------------------------------
// Initialize the grid with air and full pressure
//----------------------------------------------------------------------------------
void InitializeGrid(void) {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x].material = MATERIAL_AIR;
            grid[y][x].pressure = 1.0f;  // Start with full pressure everywhere
            grid[y][x].velocity_x = 0.0f;
            grid[y][x].velocity_y = 0.0f;
            
            // Initialize temperature gradient: warmer at bottom, cooler at top
            // Bottom (y=GRID_HEIGHT-1) = 35°C, Top (y=0) = 5°C
            float temperatureRange = 30.0f; // 35°C - 5°C
            grid[y][x].temperature = 35.0f - (temperatureRange * y / (GRID_HEIGHT - 1));
            
            nextGrid[y][x] = grid[y][x];
        }
    }
}

//----------------------------------------------------------------------------------
// Get pointer to current grid
//----------------------------------------------------------------------------------
Cell* GetGrid(void) {
    return (Cell*)grid;
}

//----------------------------------------------------------------------------------
// Get pointer to next grid
//----------------------------------------------------------------------------------
Cell* GetNextGrid(void) {
    return (Cell*)nextGrid;
}

//----------------------------------------------------------------------------------
// Swap current and next grids
//----------------------------------------------------------------------------------
void SwapGrids(void) {
    memcpy(grid, nextGrid, sizeof(grid));
}

//----------------------------------------------------------------------------------
// Check if position is within grid bounds
//----------------------------------------------------------------------------------
bool IsValidPosition(int x, int y) {
    return (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT);
}
