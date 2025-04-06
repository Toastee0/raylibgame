#include "grid.h"
#include <stdlib.h>
#include <stdio.h>
#include "cell_defaults.h"
#include "cell_types.h"

// Grid constants
int CELL_SIZE = 8;
int GRID_WIDTH = 1920 * 2 / 8; // Double the width
int GRID_HEIGHT = 1080 * 2 / 8; // Double the height

// Grid data
GridCell** grid = NULL;
GridCell* gridData = NULL; // Keep track of the original allocation

// Initialize the grid
void InitGrid(void) {
    // Allocate array of row pointers
    grid = (GridCell**)malloc(GRID_HEIGHT * sizeof(GridCell*));
    if (!grid) {
        printf("ERROR: Failed to allocate memory for grid rows\n");
        return;
    }

    // Allocate the actual grid data as a single contiguous block
    gridData = (GridCell*)malloc(GRID_HEIGHT * GRID_WIDTH * sizeof(GridCell));
    if (!gridData) {
        printf("ERROR: Failed to allocate memory for grid data\n");
        free(grid);
        grid = NULL;
        return;
    }

    // Print allocation sizes for debugging
    printf("Allocated %zu bytes for grid pointers\n", GRID_HEIGHT * sizeof(GridCell*));
    printf("Allocated %zu bytes for grid cells (%d x %d cells)\n", 
           GRID_HEIGHT * GRID_WIDTH * sizeof(GridCell), GRID_WIDTH, GRID_HEIGHT);
    printf("Size of GridCell struct: %zu bytes\n", sizeof(GridCell));

    // Initialize each cell in the grid
    for (int i = 0; i < GRID_HEIGHT; i++) {
        grid[i] = &gridData[i * GRID_WIDTH];
        for (int j = 0; j < GRID_WIDTH; j++) {
            // Use default initializer first to ensure complete initialization
            InitializeCellDefaults(&grid[i][j], CELL_TYPE_AIR);
            
            // Set position coordinates
            grid[i][j].position = (Vector2){j * CELL_SIZE, i * CELL_SIZE};
            
            // Set border cells as immutable
            if (i == 0 || i == GRID_HEIGHT - 1 || j == 0 || j == GRID_WIDTH - 1) {
                grid[i][j].type = CELL_TYPE_BORDER;
            }
        }
    }

    printf("Grid initialized successfully\n");
}

// Clean up the grid when program ends
void CleanupGrid(void) {
    if (gridData) {
        free(gridData);  // Free the actual grid data
        gridData = NULL;
    }
    
    if (grid) {
        free(grid);      // Free the array of row pointers
        grid = NULL;
    }
}

// Check if a tile is a border or out of bounds
bool IsBorderTile(int x, int y) {
    // First check bounds to avoid segfault
    if (x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT)
        return true;
        
    return (grid[y][x].type == CELL_TYPE_BORDER);
}

// Check if we can move to a tile
bool CanMoveTo(int x, int y) {
    return !IsBorderTile(x, y);
}
