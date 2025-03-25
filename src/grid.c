#include "grid.h"
#include <stdlib.h>
#include <stdio.h>
#include "src/cell_defaults.h"

// Grid constants
const int CELL_SIZE = 8;
const int GRID_WIDTH = 1920/CELL_SIZE;
const int GRID_HEIGHT = 1080/CELL_SIZE;

// Grid data
GridCell** grid = NULL;

// Initialize the grid
void InitGrid(void) {
    grid = (GridCell**)malloc(GRID_HEIGHT * sizeof(GridCell*));
    if (!grid) {
        printf("ERROR: Failed to allocate memory for grid rows\n");
        return;
    }
    
    for(int i = 0; i < GRID_HEIGHT; i++) {
        grid[i] = (GridCell*)malloc(GRID_WIDTH * sizeof(GridCell));
        if (!grid[i]) {
            printf("ERROR: Failed to allocate memory for grid row %d\n", i);
            // Clean up already allocated rows
            for (int j = 0; j < i; j++) {
                free(grid[j]);
            }
            free(grid);
            grid = NULL;
            return;
        }
        
        for(int j = 0; j < GRID_WIDTH; j++) {
            // Use the default initializer for consistent cell setup
            InitializeCellDefaults(&grid[i][j], CELL_TYPE_AIR);
            
            // Update position based on grid coordinates
            grid[i][j].position = (Vector2){j * CELL_SIZE, i * CELL_SIZE};
            
            // Make border cells immutable
            if (i == 0 || i == GRID_HEIGHT-1 || j == 0 || j == GRID_WIDTH-1) {
                grid[i][j].type = CELL_TYPE_BORDER;
            }
        }
    }
    
    printf("Grid initialized: %d x %d cells\n", GRID_WIDTH, GRID_HEIGHT);
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

// Check if a tile is a border or out of bounds
bool IsBorderTile(int x, int y) {
    return (x < 1 || x >= GRID_WIDTH - 1 || y < 1 || y >= GRID_HEIGHT - 1 || 
            grid[y][x].type == CELL_TYPE_BORDER);
}

// Check if we can move to a tile
bool CanMoveTo(int x, int y) {
    return !IsBorderTile(x, y);
}
