#include "grid.h"
#include <stdlib.h>
#include <stdio.h>

// Grid constants
const int CELL_SIZE = 8;
const int GRID_WIDTH = 1920/CELL_SIZE;
const int GRID_HEIGHT = 1080/CELL_SIZE;

// Grid data
GridCell** grid = NULL;

// Initialize the grid
void InitGrid(void) {
    grid = (GridCell**)malloc(GRID_HEIGHT * sizeof(GridCell*));
    for(int i = 0; i < GRID_HEIGHT; i++) {
        grid[i] = (GridCell*)malloc(GRID_WIDTH * sizeof(GridCell));
        for(int j = 0; j < GRID_WIDTH; j++) {
            grid[i][j].position = (Vector2){j * CELL_SIZE, i * CELL_SIZE};
            grid[i][j].baseColor = BLACK;
            
            // Initialize as air instead of vapor to prevent infinite processing on startup
            grid[i][j].type = CELL_TYPE_AIR;
            grid[i][j].moisture = 0;
            grid[i][j].permeable = 1;
            grid[i][j].is_falling = false;
            grid[i][j].volume = 0;
            
            // Initialize other fields to safe defaults
            grid[i][j].objectID = 0;
            grid[i][j].colorhigh = 0;
            grid[i][j].colorlow = 0;
            grid[i][j].Energy = 0;
            grid[i][j].height = 0;
            grid[i][j].age = 0;
            grid[i][j].maxage = 0;
            grid[i][j].temperature = 20; // Room temperature
        }
    }
}

// Clean up the grid when program ends
void CleanupGrid(void) {
    if (grid) {
        for(int i = 0; i < GRID_HEIGHT; i++) {
            if (grid[i]) {
                free(grid[i]);
            }
        }
        free(grid);
        grid = NULL;
    }
}

// Calculate total moisture in the system
int CalculateTotalMoisture(void) {
    int totalMoisture = 0;
    
    for(int y = 0; y < GRID_HEIGHT; y++) {
        for(int x = 0; x < GRID_WIDTH; x++) {
            totalMoisture += grid[y][x].moisture;
        }
    }
    
    return totalMoisture;
}

// Clamp moisture value to valid range
int ClampMoisture(int value) {
    if(value < 0) return 0;
    if(value > 100) return 100;
    return value;
}

// Check if a tile is a border or out of bounds
bool IsBorderTile(int x, int y) {
    return (x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT || 
            grid[y][x].type == CELL_TYPE_BORDER);
}

// Check if we can move to a tile
bool CanMoveTo(int x, int y) {
    return !IsBorderTile(x, y);
}
