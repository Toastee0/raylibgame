#include "grid.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>  // For memcpy
#include "cell_defaults.h"
#include "cell_types.h"
#include "updatecells.h"

// Grid constants
int CELL_SIZE = 8;
int GRID_WIDTH = 1920 * 2 / 8; // Double the width
int GRID_HEIGHT = 1080 * 2 / 8; // Double the height

// Grid data
GridCell** grid = NULL;
GridCell* gridData = NULL; // Keep track of the original allocation

// Grid serialization format version
#define GRID_FILE_VERSION 1

// File header structure for grid save files
typedef struct {
    char signature[4];         // 'SGRD' - Sandbox Grid
    int version;               // File format version
    int width;                 // Grid width
    int height;                // Grid height
    int cellSize;              // Cell size used when saving
} GridFileHeader;

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

// Save grid to a file
bool SaveGridToFile(const char* filename) {
    if (!grid || !gridData) {
        printf("ERROR: Cannot save - grid not initialized\n");
        return false;
    }
    
    FILE* file = fopen(filename, "wb");
    if (!file) {
        printf("ERROR: Failed to open file for writing: %s\n", filename);
        return false;
    }
    
    // Create and write the header
    GridFileHeader header;
    memcpy(header.signature, "SGRD", 4);
    header.version = GRID_FILE_VERSION;
    header.width = GRID_WIDTH;
    header.height = GRID_HEIGHT;
    header.cellSize = CELL_SIZE;
    
    fwrite(&header, sizeof(GridFileHeader), 1, file);
    
    // Write cell data
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            // Write only the essential properties to save space
            fwrite(&grid[y][x].type, sizeof(int), 1, file);
            fwrite(&grid[y][x].moisture, sizeof(int), 1, file);
            fwrite(&grid[y][x].baseColor, sizeof(Color), 1, file);
            fwrite(&grid[y][x].Energy, sizeof(int), 1, file);
            fwrite(&grid[y][x].age, sizeof(int), 1, file);
            fwrite(&grid[y][x].temperature, sizeof(int), 1, file);
        }
    }
    
    fclose(file);
    printf("Grid saved to: %s\n", filename);
    return true;
}

// Load grid from a file
bool LoadGridFromFile(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("ERROR: Failed to open file for reading: %s\n", filename);
        return false;
    }
    
    // Read and validate the header
    GridFileHeader header;
    if (fread(&header, sizeof(GridFileHeader), 1, file) != 1) {
        printf("ERROR: Failed to read header from file: %s\n", filename);
        fclose(file);
        return false;
    }
    
    // Check signature
    if (memcmp(header.signature, "SGRD", 4) != 0) {
        printf("ERROR: Invalid file format (wrong signature)\n");
        fclose(file);
        return false;
    }
    
    // Check version
    if (header.version != GRID_FILE_VERSION) {
        printf("ERROR: Incompatible file version: %d (expected: %d)\n", 
               header.version, GRID_FILE_VERSION);
        fclose(file);
        return false;
    }
    
    // Check grid dimensions
    if (header.width != GRID_WIDTH || header.height != GRID_HEIGHT) {
        printf("WARNING: Grid dimensions in file (%dx%d) don't match current grid (%dx%d)\n",
               header.width, header.height, GRID_WIDTH, GRID_HEIGHT);
        // We could resize the grid here, but that would be complex
        // For now, we'll just warn and continue with the current grid size
    }
    
    // If grid isn't initialized, do it now
    if (!grid || !gridData) {
        InitGrid();
    }
    
    // Clear existing grid (set all to air)
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (!(x == 0 || x == GRID_WIDTH-1 || y == 0 || y == GRID_HEIGHT-1)) {
                InitializeCellDefaults(&grid[y][x], CELL_TYPE_AIR);
                grid[y][x].position = (Vector2){x * CELL_SIZE, y * CELL_SIZE};
            }
        }
    }
    
    // Read cell data
    int maxY = (header.height < GRID_HEIGHT) ? header.height : GRID_HEIGHT;
    int maxX = (header.width < GRID_WIDTH) ? header.width : GRID_WIDTH;
    
    for (int y = 0; y < maxY; y++) {
        for (int x = 0; x < maxX; x++) {
            // Skip border cells
            if (x == 0 || x == GRID_WIDTH-1 || y == 0 || y == GRID_HEIGHT-1) {
                // Skip this cell in the file too
                fseek(file, sizeof(int)*3 + sizeof(Color) + sizeof(int)*2, SEEK_CUR);
                continue;
            }
            
            int type;
            if (fread(&type, sizeof(int), 1, file) != 1) {
                printf("ERROR: Failed to read cell data at (%d,%d)\n", x, y);
                fclose(file);
                return false;
            }
            
            // Only change the cell type if it's valid
            if (type >= CELL_TYPE_AIR && type <= CELL_TYPE_MOSS) {
                grid[y][x].type = type;
                
                // Read other properties
                fread(&grid[y][x].moisture, sizeof(int), 1, file);
                fread(&grid[y][x].baseColor, sizeof(Color), 1, file);
                fread(&grid[y][x].Energy, sizeof(int), 1, file);
                fread(&grid[y][x].age, sizeof(int), 1, file);
                fread(&grid[y][x].temperature, sizeof(int), 1, file);
                
                // Reset dynamic properties
                grid[y][x].is_falling = false;
                grid[y][x].updated_this_frame = false;
            } else {
                // Skip the rest of this cell's data
                fseek(file, sizeof(int) + sizeof(Color) + sizeof(int)*3, SEEK_CUR);
            }
        }
        
        // Skip any extra cells in each row if file is wider than current grid
        if (header.width > GRID_WIDTH) {
            fseek(file, (sizeof(int)*3 + sizeof(Color) + sizeof(int)*2) * (header.width - GRID_WIDTH), SEEK_CUR);
        }
    }
    
    // Skip any extra rows if file is taller than current grid
    fclose(file);

    printf("Grid loaded from: %s\n", filename);    return true;
}

// Main simulation update function
void UpdateGrid(void) {
    // Reset all falling states before processing movement
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x].is_falling = false;
        }
    }

    static int updateCount = 0;
    updateCount++;

    // Ensure all border cells are consistently initialized to DARKGRAY
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (x == 0 || x == GRID_WIDTH - 1 || y == 0 || y == GRID_HEIGHT - 1) {
                grid[y][x].type = CELL_TYPE_BORDER;
                grid[y][x].baseColor = DARKGRAY; // Set all border cells to DARKGRAY
            }
        }
    }

    // Update all cell types in the right order 
    updateCells();
}
