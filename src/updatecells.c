#include "updatecells.h"
#include "cell_actions.h"
#include "raylib.h"
#include <stdio.h>








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
    
    // Alternate scan direction each frame for more natural simulation
    static bool scanLeftToRight = true;
    scanLeftToRight = !scanLeftToRight;
    
    // Reset all falling states at the start of a new frame
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x].is_falling = false;
            grid[y][x].updated_this_frame = false;
        }
    }
    
    // Update border cells to ensure consistent state
    // Set the outer ring of cells to border type
    for (int x = 0; x < GRID_WIDTH; x++) {
        // Top and bottom rows
        grid[0][x].type = CELL_TYPE_BORDER;
        grid[0][x].baseColor = DARKGRAY;
        grid[0][x].updated_this_frame = true;
        
        grid[GRID_HEIGHT-1][x].type = CELL_TYPE_BORDER;
        grid[GRID_HEIGHT-1][x].baseColor = DARKGRAY;
        grid[GRID_HEIGHT-1][x].updated_this_frame = true;
    }
    
    for (int y = 0; y < GRID_HEIGHT; y++) {
        // Left and right columns
        grid[y][0].type = CELL_TYPE_BORDER;
        grid[y][0].baseColor = DARKGRAY;
        grid[y][0].updated_this_frame = true;
        
        grid[y][GRID_WIDTH-1].type = CELL_TYPE_BORDER;
        grid[y][GRID_WIDTH-1].baseColor = DARKGRAY;
        grid[y][GRID_WIDTH-1].updated_this_frame = true;
    }
    
    // Process falling cells (bottom to top)
    for (int y = GRID_HEIGHT - 2; y > 0; y--) {
        // Alternate scan direction each frame
        if (scanLeftToRight) {
            for (int x = 1; x < GRID_WIDTH - 1; x++) {
                if (!grid[y][x].updated_this_frame && 
                    (grid[y][x].type == CELL_TYPE_SOIL || 
                     grid[y][x].type == CELL_TYPE_WATER ||
                     grid[y][x].type == CELL_TYPE_PLANT ||
                     grid[y][x].type == CELL_TYPE_MOSS)) {
                    updateCell(x, y);
                }
            }
        } else {
            for (int x = GRID_WIDTH - 2; x > 0; x--) {
                if (!grid[y][x].updated_this_frame && 
                    (grid[y][x].type == CELL_TYPE_SOIL || 
                     grid[y][x].type == CELL_TYPE_WATER ||
                     grid[y][x].type == CELL_TYPE_PLANT ||
                     grid[y][x].type == CELL_TYPE_MOSS)) {
                    updateCell(x, y);
                }
            }
        }
    }
    
    // Process rising cells (top to bottom)
    for (int y = 1; y < GRID_HEIGHT - 1; y++) {
        // Alternate scan direction each frame here too
        if (scanLeftToRight) {
            for (int x = 1; x < GRID_WIDTH - 1; x++) {
                if (!grid[y][x].updated_this_frame && grid[y][x].type == CELL_TYPE_AIR) {
                    updateAirCell(x, y);
                }
            }
        } else {
            for (int x = GRID_WIDTH - 2; x > 0; x--) {
                if (!grid[y][x].updated_this_frame && grid[y][x].type == CELL_TYPE_AIR) {
                    updateAirCell(x, y);
                }
            }
        }
    }
    
    // New pass: update columns for cohesive falling movement
    for (int x = 1; x < GRID_WIDTH - 1; x++) {
        for (int y = GRID_HEIGHT - 2; y > 1; y--) {
            if (grid[y][x].type == CELL_TYPE_AIR) {
                int t;
                // Scan upward from the air cell to find a falling cell
                for (t = y - 1; t > 0; t--) {
                    if (grid[t][x].type != CELL_TYPE_AIR && grid[t][x].is_falling) {
                        break;
                    }
                }
                if (t > 0) {
                    SwapCells(x, t, x, y);
                    grid[y][x].updated_this_frame = true;
                    grid[t][x].updated_this_frame = true;
                }
            }
        }
    }
}

// Route each cell to its appropriate update function
void updateCell(int x, int y) {
    if (IsBorderTile(x, y) || grid[y][x].updated_this_frame) return;
    
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
        case CELL_TYPE_AIR:
            // Air is handled in a separate pass, but we can call it here if needed
            break;
        case CELL_TYPE_ROCK:
            // Rocks don't move or update
            break;
        case CELL_TYPE_BORDER:
            // Border cells don't update
            break;
    }
    
    grid[y][x].updated_this_frame = true;
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
    
    // Try falling motion first - straight down is preferred
    if (moveDirs & DIR_DOWN) {
        if (tryMoveInDirection(x, y, DIR_DOWN)) {
            return;
        }
    }
    
    // Try diagonal falling
    bool fell = false;
    if ((moveDirs & DIR_DOWN_LEFT) && (moveDirs & DIR_DOWN_RIGHT)) {
        // Choose randomly between the two diagonals
        if (GetRandomValue(0, 1) == 0) {
            fell = tryMoveInDirection(x, y, DIR_DOWN_LEFT);
        } else {
            fell = tryMoveInDirection(x, y, DIR_DOWN_RIGHT);
        }
    } else if (moveDirs & DIR_DOWN_LEFT) {
        fell = tryMoveInDirection(x, y, DIR_DOWN_LEFT);
    } else if (moveDirs & DIR_DOWN_RIGHT) {
        fell = tryMoveInDirection(x, y, DIR_DOWN_RIGHT);
    }
    
    // If cell didn't fall, try to slide horizontally based on slope
    if (!fell) {
        bool isOnSlope = false;
        bool slopeLeft = false;
        
        // Check if cell is on a slope
        if ((grid[y+1][x-1].type == CELL_TYPE_AIR || grid[y+1][x-1].type == CELL_TYPE_WATER) && 
            (grid[y+1][x].type != CELL_TYPE_AIR && grid[y+1][x].type != CELL_TYPE_WATER)) {
            isOnSlope = true;
            slopeLeft = true;
        } else if ((grid[y+1][x+1].type == CELL_TYPE_AIR || grid[y+1][x+1].type == CELL_TYPE_WATER) && 
                  (grid[y+1][x].type != CELL_TYPE_AIR && grid[y+1][x].type != CELL_TYPE_WATER)) {
            isOnSlope = true;
            slopeLeft = false;
        }
        
        // Try to slide on slope
        if (isOnSlope) {
            if (slopeLeft && (moveDirs & DIR_LEFT)) {
                tryMoveInDirection(x, y, DIR_LEFT);
            } else if (!slopeLeft && (moveDirs & DIR_RIGHT)) {
                tryMoveInDirection(x, y, DIR_RIGHT);
            }
        }
    }
    
    // Soil moisture diffusion to be implemented
}

// Update water cell physics and behavior
void updateWaterCell(int x, int y) {
    // Get valid movement directions for water
    unsigned char moveDirs = getValidDirections(x, y, CELL_TYPE_WATER);
    
    // Update water color based on moisture level
    float moistureRatio = (float)grid[y][x].moisture / 100.0f;
    grid[y][x].baseColor = (Color){
        0 + (int)(200 * (1.0f - moistureRatio)),
        120 + (int)(135 * (1.0f - moistureRatio)),
        255,
        255
    };
    
    // Try falling motion first - straight down is preferred
    if (moveDirs & DIR_DOWN) {
        if (tryMoveInDirection(x, y, DIR_DOWN)) {
            return;
        }
    }
    
    // Try diagonal falling
    bool fell = false;
    if ((moveDirs & DIR_DOWN_LEFT) && (moveDirs & DIR_DOWN_RIGHT)) {
        // Choose randomly between the two diagonals
        if (GetRandomValue(0, 1) == 0) {
            fell = tryMoveInDirection(x, y, DIR_DOWN_LEFT);
        } else {
            fell = tryMoveInDirection(x, y, DIR_DOWN_RIGHT);
        }
    } else if (moveDirs & DIR_DOWN_LEFT) {
        fell = tryMoveInDirection(x, y, DIR_DOWN_LEFT);
    } else if (moveDirs & DIR_DOWN_RIGHT) {
        fell = tryMoveInDirection(x, y, DIR_DOWN_RIGHT);
    }
    
    // If water didn't fall, try to spread horizontally
    if (!fell) {
        // Check both sides first
        bool canMoveLeft = (moveDirs & DIR_LEFT);
        bool canMoveRight = (moveDirs & DIR_RIGHT);
        
        // If both directions are available, choose randomly with a bias
        // towards the direction with more space for the water to flow
        if (canMoveLeft && canMoveRight) {
            // Count available cells in each direction
            int leftSpace = 0, rightSpace = 0;
            for (int i = 1; i <= 5; i++) { // Look up to 5 cells in each direction
                if (x-i >= 0 && grid[y][x-i].type == CELL_TYPE_AIR) leftSpace++;
                if (x+i < GRID_WIDTH && grid[y][x+i].type == CELL_TYPE_AIR) rightSpace++;
            }
            
            // Add some randomness but bias toward direction with more space
            if (GetRandomValue(0, leftSpace + rightSpace) < leftSpace) {
                tryMoveInDirection(x, y, DIR_LEFT);
            } else {
                tryMoveInDirection(x, y, DIR_RIGHT);
            }
        } else if (canMoveLeft) {
            tryMoveInDirection(x, y, DIR_LEFT);
        } else if (canMoveRight) {
            tryMoveInDirection(x, y, DIR_RIGHT);
        }
        
        // Water evaporation will be implemented here
    }
}

// Update air cell physics (including moisture and clouds)
void updateAirCell(int x, int y) {
    // Air tends to rise
    unsigned char moveDirs = getValidDirections(x, y, CELL_TYPE_AIR);
    
    // Update air color based on moisture
    if (grid[y][x].moisture > 75) {
        int brightness = (grid[y][x].moisture - 75) * (255 / 25);
        grid[y][x].baseColor = (Color){brightness, brightness, brightness, 255};
    } else {
        grid[y][x].baseColor = BLACK;  // Invisible air
    }
    
    // Only move air cells with high moisture (rising vapor)
    if (grid[y][x].moisture > 50) {
        // Try rising straight up
        if (moveDirs & DIR_UP) {
            if (tryMoveInDirection(x, y, DIR_UP)) {
                return;
            }
        }
        
        // Try diagonal rising
        if ((moveDirs & DIR_UP_LEFT) && (moveDirs & DIR_UP_RIGHT)) {
            // Choose randomly between the two diagonals
            if (GetRandomValue(0, 1) == 0) {
                tryMoveInDirection(x, y, DIR_UP_LEFT);
            } else {
                tryMoveInDirection(x, y, DIR_UP_RIGHT);
            }
        } else if (moveDirs & DIR_UP_LEFT) {
            tryMoveInDirection(x, y, DIR_UP_LEFT);
        } else if (moveDirs & DIR_UP_RIGHT) {
            tryMoveInDirection(x, y, DIR_UP_RIGHT);
        }
        
        // Horizontal movement of air (wind) will be implemented here
    }
    
    // Moisture diffusion between air cells
    for (int i = 0; i < 8; i++) {
        int nx = x + DIR_X[i];
        int ny = y + DIR_Y[i];
        
        if (nx < 1 || nx >= GRID_WIDTH-1 || ny < 1 || ny >= GRID_HEIGHT-1) continue;
        
        if (grid[ny][nx].type == CELL_TYPE_AIR && !grid[ny][nx].updated_this_frame) {
            int moistureDiff = grid[y][x].moisture - grid[ny][nx].moisture;
            if (moistureDiff > 1) {
                int transfer = moistureDiff / 8;  // Distribute gradually
                if (transfer > 0) {
                    tryMoistureDiffusion(x, y, 1 << i, transfer);
                }
            }
        }
    }
    
    // Cloud formation and precipitation will be implemented here
    if (grid[y][x].moisture > 95) {
        // Chance to form water droplet (precipitation)
        if (GetRandomValue(0, 100) < 5) {
            grid[y][x].type = CELL_TYPE_WATER;
            grid[y][x].moisture = 100;
            grid[y][x].baseColor = (Color){20, 120, 255, 255};
        }
    }
}

// Update plant cell growth and interactions
void updatePlantCell(int x, int y) {
    // Plants can fall if not supported
    unsigned char moveDirs = getValidDirections(x, y, CELL_TYPE_PLANT);
    
    // Aging
    grid[y][x].age++;
    
    // Plants can fall
    if (moveDirs & DIR_DOWN) {
        tryMoveInDirection(x, y, DIR_DOWN);
        return;
    }
    
    // Moisture absorption from nearby cells
    for (int i = 0; i < 8; i++) {
        int nx = x + DIR_X[i];
        int ny = y + DIR_Y[i];
        
        if (nx < 1 || nx >= GRID_WIDTH-1 || ny < 1 || ny >= GRID_HEIGHT-1) continue;
        
        if (grid[ny][nx].type == CELL_TYPE_WATER || grid[ny][nx].type == CELL_TYPE_SOIL) {
            if (grid[ny][nx].moisture > 20 && grid[y][x].moisture < 80) {
                int transfer = GetRandomValue(1, 5);
                tryMoistureDiffusion(nx, ny, 1 << ((i + 4) % 8), transfer); // Opposite direction
            }
        }
    }
    
    // Plant growth chance based on age and moisture
    if (grid[y][x].age > 100 && grid[y][x].moisture > 40) {
        // Check for empty space to grow
        unsigned char emptyDirs = getEmptyDirections(x, y);
        if (emptyDirs) {
            // Select a random direction with preference for upward growth
            int dirToGrow = -1;
            int totalWeight = 0;
            int weights[8] = {1, 3, 1, 1, 1, 1, 1, 1}; // Higher weight for upward
            
            for (int i = 0; i < 8; i++) {
                if (emptyDirs & (1 << i)) {
                    totalWeight += weights[i];
                }
            }
            
            int choice = GetRandomValue(1, totalWeight);
            int currentWeight = 0;
            
            for (int i = 0; i < 8; i++) {
                if (emptyDirs & (1 << i)) {
                    currentWeight += weights[i];
                    if (choice <= currentWeight) {
                        dirToGrow = i;
                        break;
                    }
                }
            }
            
            if (dirToGrow >= 0) {
                int nx = x + DIR_X[dirToGrow];
                int ny = y + DIR_Y[dirToGrow];
                
                if (nx >= 1 && nx < GRID_WIDTH-1 && ny >= 1 && ny < GRID_HEIGHT-1 && 
                    grid[ny][nx].type == CELL_TYPE_AIR) {
                    grid[ny][nx].type = CELL_TYPE_PLANT;
                    grid[ny][nx].baseColor = (Color){20, 200, 20, 255};
                    grid[ny][nx].age = 0;
                    grid[ny][nx].moisture = 20;
                    grid[ny][nx].updated_this_frame = true;
                    
                    // Consume moisture for growth
                    grid[y][x].moisture -= 10;
                }
            }
        }
    }
}

// Update moss cell behavior
void updateMossCell(int x, int y) {
    // Moss can fall if not supported
    unsigned char moveDirs = getValidDirections(x, y, CELL_TYPE_MOSS);
    
    // Aging
    grid[y][x].age++;
    
    // Moss can fall
    if (moveDirs & DIR_DOWN) {
        tryMoveInDirection(x, y, DIR_DOWN);
        return;
    }
    
    // Moss spreads differently than plants - prefers surfaces
    if (grid[y][x].age > 150 && grid[y][x].moisture > 30) {
        // Try to find adjacent surfaces to spread onto
        for (int i = 0; i < 8; i++) {
            int nx = x + DIR_X[i];
            int ny = y + DIR_Y[i];
            
            if (nx < 1 || nx >= GRID_WIDTH-1 || ny < 1 || ny >= GRID_HEIGHT-1) continue;
            
            // Moss spreads to rock or soil
            if ((grid[ny][nx].type == CELL_TYPE_ROCK || grid[ny][nx].type == CELL_TYPE_SOIL) && 
                !grid[ny][nx].updated_this_frame) {
                // Check if there's air adjacent to the target cell (moss needs some air)
                bool hasAirAdjacent = false;
                for (int j = 0; j < 8; j++) {
                    int nnx = nx + DIR_X[j];
                    int nny = ny + DIR_Y[j];
                    
                    if (nnx < 1 || nnx >= GRID_WIDTH-1 || nny < 1 || nny >= GRID_HEIGHT-1) continue;
                    
                    if (grid[nny][nnx].type == CELL_TYPE_AIR) {
                        hasAirAdjacent = true;
                        break;
                    }
                }
                
                // Spread if conditions are good
                if (hasAirAdjacent && GetRandomValue(0, 100) < 5) {
                    grid[ny][nx].type = CELL_TYPE_MOSS;
                    grid[ny][nx].baseColor = (Color){20, 180, 20, 255};
                    grid[ny][nx].age = 0;
                    grid[ny][nx].moisture = 20;
                    grid[ny][nx].updated_this_frame = true;
                    
                    // Consume moisture for growth
                    grid[y][x].moisture -= 5;
                    break;  // Only spread to one cell per update
                }
            }
        }
    }
}

// Get valid movement directions based on cell type and surroundings
unsigned char getValidDirections(int x, int y, int cellType) {
    unsigned char dirs = 0;
    
    // Check each direction
    for (int i = 0; i < 8; i++) {
        int nx = x + DIR_X[i];
        int ny = y + DIR_Y[i];
        
        // Skip if out of bounds
        if (nx < 1 || nx >= GRID_WIDTH-1 || ny < 1 || ny >= GRID_HEIGHT-1) {
            continue;
        }
        
        // Skip if cell has already been updated this frame
        if (grid[ny][nx].updated_this_frame) {
            continue;
        }
        
        // Different rules for different cell types
        switch (cellType) {
            case CELL_TYPE_SOIL:
                // Soil can move into air or water
                if (grid[ny][nx].type == CELL_TYPE_AIR || 
                    grid[ny][nx].type == CELL_TYPE_WATER) {
                    dirs |= (1 << i);
                }
                break;
                
            case CELL_TYPE_WATER:
                // Water can move into air or displace other water
                if (grid[ny][nx].type == CELL_TYPE_AIR ||
                    (grid[ny][nx].type == CELL_TYPE_WATER && 
                     grid[ny][nx].moisture < grid[y][x].moisture)) {
                    dirs |= (1 << i);
                }
                break;
                
            case CELL_TYPE_AIR:
                // Air with high moisture can rise through other air
                if (grid[ny][nx].type == CELL_TYPE_AIR &&
                    grid[y][x].moisture > grid[ny][nx].moisture + 10) {
                    dirs |= (1 << i);
                }
                break;
                
            case CELL_TYPE_PLANT:
            case CELL_TYPE_MOSS:
                // Plants and moss can fall into air
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
        if (nx < 1 || nx >= GRID_WIDTH-1 || ny < 1 || ny >= GRID_HEIGHT-1) {
            continue;
        }
        
        if (grid[ny][nx].type == CELL_TYPE_AIR && !grid[ny][nx].updated_this_frame) {
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
    
    // Skip if the target is out of bounds or already updated
    if (nx < 1 || nx >= GRID_WIDTH-1 || ny < 1 || ny >= GRID_HEIGHT-1 || grid[ny][nx].updated_this_frame) {
        return false;
    }
    
    // Check if target cell can be modified based on type
    if (grid[ny][nx].type == CELL_TYPE_BORDER || grid[ny][nx].type == CELL_TYPE_ROCK) {
        return false;
    }
    
    // Special case for water-water interaction
    if (grid[y][x].type == CELL_TYPE_WATER && grid[ny][nx].type == CELL_TYPE_WATER) {
        // Combine water if possible
        int totalMoisture = grid[y][x].moisture + grid[ny][nx].moisture;
        if (totalMoisture <= 100) {
            // Merge water cells
            grid[ny][nx].moisture = totalMoisture;
            grid[y][x].type = CELL_TYPE_AIR;
            grid[y][x].moisture = 10; // Leave some moisture in the air
            grid[y][x].baseColor = BLACK;
            grid[y][x].updated_this_frame = true;
            grid[ny][nx].updated_this_frame = true;
            return true;
        } else if (grid[y][x].moisture > grid[ny][nx].moisture + 10) {
            // Partial exchange of water
            int transfer = (grid[y][x].moisture - grid[ny][nx].moisture) / 2;
            grid[ny][nx].moisture += transfer;
            grid[y][x].moisture -= transfer;
            grid[y][x].updated_this_frame = true;
            grid[ny][nx].updated_this_frame = true;
            return true;
        }
        return false;
    }
    
    // Handle generic cell movement
    SwapCells(x, y, nx, ny);
    grid[ny][nx].is_falling = true;
    grid[y][x].updated_this_frame = true;
    grid[ny][nx].updated_this_frame = true;
    
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