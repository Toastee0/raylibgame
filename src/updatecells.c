#include "updatecells.h"
#include "cell_actions.h"
#include "raylib.h"
#include <stdio.h>

//sim rules.
// cells do not "move" the properties of cells "move", so we never update the position of a cell, as it doesn't change. but the contents of that cell may change, and that gives the illusion of movement.
// moisture is conserved, and only transacted in integer amounts. this is to prevent floating point errors from causing issues with the simulation. we can always convert to float for display purposes, but we should keep the internal representation as integers.
// cells are defined in cell_types.h, and the grid is defined in grid.h. the grid is a 2D array of cells, and each cell has a type, moisture, temperature, age, and color. the color is used for display purposes only, and is not used for any calculations.
// the age is used to determine if a cell can reproduce or die.
// the temperature is used to determine if a cell can evaporate or freeze. 
// moisture and moisturecapacity are used to determine if a cell can take, or give moisture to another cell. moisture is a value between 0 and moisturecapacity. 
// air holds 100 units of moistute, water holes 1000 units of moisture meaing that it takes 10 units of air to condense into 1 cell of water.
// water with no moisture remaining becomes air type.
// moisture does double duty as the density value for the cell. if two water cells are touching, and their sum total would be less than the moisture capacity of a single cell, then will merge into a single cell.
// partial water transfer is also possible, but leaves a cell with whatever the small moisture remainder is. we always transfer in integer amounts to avoid floating point errors.

//potential update pattern variations.
// randomly selectiong left/right per row
// checkerboard pattern

//for certain types we want to know if it's part of a bigger group of the same type. this allows us to partition off sections of the grid as "gas" or "liquid" regions that allows us to process things like pressure, and have objects move in clumps if we choose to.
//to allow for clumping in certain cell types like soil. we can walk the grid to generate sub-arrays that represent the clumps of cells. this will allow us to process them as a single unit, and allow for more complex interactions between them. we can also use this to determine if a cell is part of a larger group of the same type, and if so, we can process them as a single unit. this will allow us to process things like pressure, and have objects move in clumps if we choose to.
//example for soil, it would clump more in a certain moisture range, but when saturated, or quite dry it would flow or be pushed.

//example for water vs soil. if we have a large cluster of water, and it's got a column of sand to it's left or right, that sand if it's really thin, say a few blocks can be saturated, and then start to slump as the water pushes it  out of the way. 
// a larger amount of sand would be able to prevent the water from moving it, but moisture could slowly transfer through the sand, eventually condensing into water droplets on the surface of the sand as the water will prefer to travel horizontally through soil via a diffusion like mechanism
//the diffusion mechanism will be impimmented using integers, and not take or give mor water than should exist to keep the sim stable.
// this will be done by only transfering amounts that exist, and that we have checked that the destinatiion cell had room for.

//plan for update cells.c
// first step is to determine if the cell needs to be supported. this is dependant on cell type, and if we have neighbors that we are attracted to so each cell should have a preset bit mask of regions that it checks for support.
// 
// if we find that we can move, we check based on the bitmask which of the available directions is prefered based on the rules for this cell type.
















// Direction offset arrays
const int DIR_X[8] = {-1,  0,  1, -1,  1, -1,  0,  1};
const int DIR_Y[8] = {-1, -1, -1,  0,  0,  1,  1,  1};

// Main update function that manages all cell types
void updateCells(void) {
    // Check if grid is initialized first to prevent null pointer access
    if (grid == NULL) {
        printf("ERROR: Attempted to update cells with uninitialized grid\n");
        return;
    }
    
    // Process from bottom to top for gravity-affected cells
    // Use alternating scan directions each frame for more natural results
    static bool scanLeftToRight = true;
    scanLeftToRight = !scanLeftToRight;
    
    // Reset all falling states at the start of a new frame - SAFELY
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            // Skip the boundary check here - we just want to reset all cells
            grid[y][x].is_falling = false;
        }
    }
    
    // Update border cells to ensure consistent state
    // Set the outer ring of cells to border type
    for (int x = 0; x < GRID_WIDTH; x++) {
        // Top and bottom rows
        grid[0][x].type = CELL_TYPE_BORDER;
        grid[0][x].baseColor = DARKGRAY;
        
        grid[GRID_HEIGHT-1][x].type = CELL_TYPE_BORDER;
        grid[GRID_HEIGHT-1][x].baseColor = DARKGRAY;
    }
    
    for (int y = 0; y < GRID_HEIGHT; y++) {
        // Left and right columns
        grid[y][0].type = CELL_TYPE_BORDER;
        grid[y][0].baseColor = DARKGRAY;
        
        grid[y][GRID_WIDTH-1].type = CELL_TYPE_BORDER;
        grid[y][GRID_WIDTH-1].baseColor = DARKGRAY;
    }
    
    // Process cells from bottom to top for gravity effects
    // Only process non-border cells (from 1 to GRID_HEIGHT-2 and 1 to GRID_WIDTH-2)
    for (int y = GRID_HEIGHT - 2; y > 0; y--) {
        if (scanLeftToRight) {
            for (int x = 1; x < GRID_WIDTH - 1; x++) {
                updateCell(x, y);
            }
        } else {
            for (int x = GRID_WIDTH - 2; x > 0; x--) {
                updateCell(x, y);
            }
        }
    }
    
    // Second pass for air movement (top to bottom)
    for (int y = 1; y < GRID_HEIGHT - 1; y++) {
        for (int x = 1; x < GRID_WIDTH - 1; x++) {
            if (grid[y][x].type == CELL_TYPE_AIR) {
                updateAirCell(x, y);
            }
        }
    }
}

// Route each cell to its appropriate update function
void updateCell(int x, int y) {
    if (IsBorderTile(x, y)) return;
    
    switch (grid[y][x].type) {
        case CELL_TYPE_SOIL:
            updateSoilCell(x, y);
            break;
        case CELL_TYPE_WATER:
            updateWaterCell(x, y);
            break;
        case CELL_TYPE_PLANT:
            updatePlantCell(x, y);
            break;
        case CELL_TYPE_MOSS:
            updateMossCell(x, y);
            break;
        // Air is handled in a separate pass
        case CELL_TYPE_ROCK:
        case CELL_TYPE_BORDER:
            // These cell types don't update
            break;
    }
}

// Update soil cell physics and behavior
void updateSoilCell(int x, int y) {
    // Get allowed movement directions based on physics rules
    unsigned char moveDirs = getValidDirections(x, y, CELL_TYPE_SOIL);
    
    // Update soil color based on moisture
    float moistureRatio = (float)grid[y][x].moisture / 100.0f;
    grid[y][x].baseColor = (Color){
        127 - (moistureRatio * 51),
        106 - (moistureRatio * 43),
        79 - (moistureRatio * 32),
        255
    };
    
    // Try to move down first
    if (moveDirs & DIR_DOWN) {
        tryMoveInDirection(x, y, DIR_DOWN);
        return;
    }
    
    // Try diagonal down movement if couldn't move straight down
    if (moveDirs & DIR_DOWN_LEFT && moveDirs & DIR_DOWN_RIGHT) {
        // Choose randomly between the two diagonals
        if (GetRandomValue(0, 1) == 0) {
            tryMoveInDirection(x, y, DIR_DOWN_LEFT);
        } else {
            tryMoveInDirection(x, y, DIR_DOWN_RIGHT);
        }
        return;
    } else if (moveDirs & DIR_DOWN_LEFT) {
        tryMoveInDirection(x, y, DIR_DOWN_LEFT);
        return;
    } else if (moveDirs & DIR_DOWN_RIGHT) {
        tryMoveInDirection(x, y, DIR_DOWN_RIGHT);
        return;
    }
    
    // Soil moisture diffusion will be implemented here
}

// Update water cell physics and behavior
void updateWaterCell(int x, int y) {
    // Get valid movement directions for water
    unsigned char moveDirs = getValidDirections(x, y, CELL_TYPE_WATER);
    
    // Update water color based on moisture
    float moistureRatio = (float)grid[y][x].moisture / 100.0f;
    grid[y][x].baseColor = (Color){
        0 + (int)(200 * (1.0f - moistureRatio)),
        120 + (int)(135 * (1.0f - moistureRatio)),
        255,
        255
    };
    
    // Similar to soil, try to move down first
    if (moveDirs & DIR_DOWN) {
        tryMoveInDirection(x, y, DIR_DOWN);
        return;
    }
    
    // Try diagonal down movement
    if (moveDirs & DIR_DOWN_LEFT && moveDirs & DIR_DOWN_RIGHT) {
        if (GetRandomValue(0, 1) == 0) {
            tryMoveInDirection(x, y, DIR_DOWN_LEFT);
        } else {
            tryMoveInDirection(x, y, DIR_DOWN_RIGHT);
        }
        return;
    } else if (moveDirs & DIR_DOWN_LEFT) {
        tryMoveInDirection(x, y, DIR_DOWN_LEFT);
        return;
    } else if (moveDirs & DIR_DOWN_RIGHT) {
        tryMoveInDirection(x, y, DIR_DOWN_RIGHT);
        return;
    }
    
    // If water can't move down, try to spread horizontally
    if (moveDirs & DIR_LEFT && moveDirs & DIR_RIGHT) {
        if (GetRandomValue(0, 1) == 0) {
            tryMoveInDirection(x, y, DIR_LEFT);
        } else {
            tryMoveInDirection(x, y, DIR_RIGHT);
        }
    } else if (moveDirs & DIR_LEFT) {
        tryMoveInDirection(x, y, DIR_LEFT);
    } else if (moveDirs & DIR_RIGHT) {
        tryMoveInDirection(x, y, DIR_RIGHT);
    }
    
    // Water evaporation will be implemented here
}

// Update air cell physics (including moisture and clouds)
void updateAirCell(int x, int y) {
    // Air behavior is mostly about moisture movement and cloud formation
    // This is just a framework - we'll implement the details later
    
    // Update air color based on moisture
    if (grid[y][x].moisture > 75) {
        int brightness = (grid[y][x].moisture - 75) * (255 / 25);
        grid[y][x].baseColor = (Color){brightness, brightness, brightness, 255};
    } else {
        grid[y][x].baseColor = BLACK;  // Invisible air
    }
    
    // Moisture diffusion between air cells will be implemented here
    
    // Cloud formation logic will be implemented here
}

// Update plant cell growth and interactions
void updatePlantCell(int x, int y) {
    // Plants mostly interact with moisture and occasionally grow or reproduce
    // This is just a framework - we'll implement the details later
    
    // Aging
    grid[y][x].age++;
    
    // Moisture absorption from nearby cells will be implemented here
    
    // Growth and reproduction will be implemented here
}

// Update moss cell behavior
void updateMossCell(int x, int y) {
    // Similar to plants but with different growth patterns
    // This is just a framework - we'll implement the details later
    
    // Aging
    grid[y][x].age++;
    
    // Moisture absorption will be implemented here
    
    // Growth and spread will be implemented here
}

// Get valid movement directions based on cell type and surroundings
unsigned char getValidDirections(int x, int y, int cellType) {
    unsigned char dirs = 0;
    
    // Check each direction
    for (int i = 0; i < 8; i++) {
        int nx = x + DIR_X[i];
        int ny = y + DIR_Y[i];
        
        // Skip if out of bounds
        if (nx < 0 || nx > GRID_WIDTH - 1 || ny < 0 || ny > GRID_HEIGHT - 1) {
            continue;
        }
        
        // Different rules for different cell types
        switch (cellType) {
            case CELL_TYPE_SOIL:
            case CELL_TYPE_WATER:
                // These can move into air or water (with rules)
                if (grid[ny][nx].type == CELL_TYPE_AIR || 
                    grid[ny][nx].type == CELL_TYPE_WATER) {
                    dirs |= (1 << i);
                }
                break;
                
            case CELL_TYPE_AIR:
                // Air movement rules are more complex
                // Will be implemented with moisture considerations
                if (grid[ny][nx].type == CELL_TYPE_AIR) {
                    dirs |= (1 << i);
                }
                break;
                
            default:
                break;
        }
    }
    
    return dirs;
}

// Get empty directions (air cells)
unsigned char getEmptyDirections(int x, int y) {
    unsigned char dirs = 0;
    
    // Check each direction
    for (int i = 0; i < 8; i++) {
        int nx = x + DIR_X[i];
        int ny = y + DIR_Y[i];
        
        // Skip if out of bounds
        if (nx < 0 || nx > GRID_WIDTH - 1 || ny < 0 || ny > GRID_HEIGHT - 1) {
            continue;
        }
        
        if (grid[ny][nx].type == CELL_TYPE_AIR) {
            dirs |= (1 << i);
        }
    }
    
    return dirs;
}

// Get directions where moisture exceeds threshold
unsigned char getMoistureDirections(int x, int y, int threshold) {
    unsigned char dirs = 0;
    
    // Check each direction
    for (int i = 0; i < 8; i++) {
        int nx = x + DIR_X[i];
        int ny = y + DIR_Y[i];
        
        // Skip if out of bounds
        if (nx < 0 || nx > GRID_WIDTH - 1 || ny < 0 || ny > GRID_HEIGHT - 1) {
            continue;
        }
        
        if (grid[ny][nx].moisture > threshold) {
            dirs |= (1 << i);
        }
    }
    
    return dirs;
}

// Try to move in a specific direction
bool tryMoveInDirection(int x, int y, unsigned char direction) {
    // Find the bit index
    int dirIndex = 0;
    unsigned char dirBit = direction;
    
    while (dirBit > 1) {
        dirBit >>= 1;
        dirIndex++;
    }
    
    int nx = x + DIR_X[dirIndex];
    int ny = y + DIR_Y[dirIndex];
    
    // Skip if the target is out of bounds
    if (nx < 0 || nx > GRID_WIDTH - 1 || ny < 0 || ny > GRID_HEIGHT - 1) {
        return false;
    }
    
    // Perform the move
    MoveCell(x, y, nx, ny);
    grid[ny][nx].is_falling = true;
    
    return true;
}

// Try to diffuse moisture in a specific direction
bool tryMoistureDiffusion(int x, int y, unsigned char direction, int amount) {
    // Find the bit index
    int dirIndex = 0;
    unsigned char dirBit = direction;
    
    while (dirBit > 1) {
        dirBit >>= 1;
        dirIndex++;
    }
    
    int nx = x + DIR_X[dirIndex];
    int ny = y + DIR_Y[dirIndex];
    
    // Skip if the target is out of bounds
    if (nx < 0 || nx > GRID_WIDTH - 1 || ny < 0 || ny > GRID_HEIGHT - 1) {
        return false;
    }
    
    // Calculate how much moisture can be transferred
    int available = grid[y][x].moisture;
    int maxAbsorb = 100 - grid[ny][nx].moisture;
    int transfer = (amount < available) ? amount : available;
    transfer = (transfer < maxAbsorb) ? transfer : maxAbsorb;
    
    // Only perform transfer if there's something to transfer
    if (transfer > 0) {
        grid[y][x].moisture -= transfer;
        grid[ny][nx].moisture += transfer;
        return true;
    }
    
    return false;
}