#include "simulation.h"
#include "grid.h"
#include "cell_types.h"
#include "cell_actions.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>


//AbsorbMoisture helper function
void AbsorbMoisture(int* source, int* target) {
    int canabsorb = *source - (100 - *target);
    int leftover = *source - canabsorb;
    *target += canabsorb;
    *source = leftover;
}



// Helper function to count neighboring water cells
int CountWaterNeighbors(int x, int y) {
    int count = 0;
    for(int dy = -1; dy <= 1; dy++) {
        for(int dx = -1; dx <= 1; dx++) {
            if(dx == 0 && dy == 0) continue;
            
            int nx = x + dx;
            int ny = y + dy;
            if(!IsBorderTile(nx, ny) && grid[ny][nx].type == CELL_TYPE_WATER) {
                count++;
            }
        }
    }
    return count;
}

// Main simulation update function
void UpdateGrid(void) {
    // Reset all falling states before processing movement
    for(int y = 0; y < GRID_HEIGHT; y++) {
        for(int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x].is_falling = false;
        }
    }
    
    static int updateCount = 0;
    updateCount++;
    
   
        UpdateWater();
        UpdateSoil();
}

// Update soil physics
//checks if it can absorb from under it, if it can, it does so, if it can fall through the material below it, it does so.
//soil can absorb moisture from water and air, and can fall through water and air.
//if it fell, we set the falling flag, and skip the rest of the checks.
//if it didn't fall, we check check if we can move down to the right or down to the left, if we can, we move there, and set the falling flag. skipping subsequent checks.


void UpdateSoil(void) {
    // Randomly decide initial direction for this cycle
    bool processRightToLeft = GetRandomValue(0, 1);
    
    // Process soil from bottom to top, alternating left/right direction
    for(int y = GRID_HEIGHT - 1; y >= 0; y--) {
        // Alternate direction for each row
        processRightToLeft = !processRightToLeft;
        
        int startX, endX, stepX;
        if(processRightToLeft) {
            // Right to left
            startX = GRID_WIDTH - 1;
            endX = -1;
            stepX = -1;
        } else {
            // Left to right
            startX = 0;
            endX = GRID_WIDTH;
            stepX = 1;
        }
        
        for(int x = startX; x != endX; x += stepX) {
            if(grid[y][x].type == CELL_TYPE_SOIL) {
                // Reset falling state before movement logic
                grid[y][x].is_falling = false;
                bool hasMoved = false;
                
                // Track soil moisture
                int* moisture = &grid[y][x].moisture;

                // Update soil color based on moisture
                float intensityPct = (float)*moisture / 100.0f;
                grid[y][x].baseColor = (Color){
                    127 - (intensityPct * 51),
                    106 - (intensityPct * 43),
                    79 - (intensityPct * 32),
                    255
                };

                // Check if soil can fall straight down
                if(y < GRID_HEIGHT - 1) {
                    if(grid[y+1][x].type == CELL_TYPE_AIR || grid[y+1][x].type == CELL_TYPE_WATER) {
                        // Transfer moisture if falling thru water
                        if(*moisture < 100 && grid[y+1][x].type == CELL_TYPE_WATER) {
                            AbsorbMoisture(&grid[y+1][x].moisture, moisture);
                        }
                        
                        // Actually move the soil cell down
                        MoveCell(x, y, x, y+1);
                        grid[y+1][x].is_falling = true;
                        hasMoved = true;
                        continue;  // Skip further checks, we've moved
                    }
                }

                // Check if soil can fall diagonally
                if(y < GRID_HEIGHT - 1 && !hasMoved) {
                    bool canMoveLeft = (x > 0 && (grid[y+1][x-1].type == CELL_TYPE_AIR || 
                                                 grid[y+1][x-1].type == CELL_TYPE_WATER));
                    bool canMoveRight = (x < GRID_WIDTH-1 && (grid[y+1][x+1].type == CELL_TYPE_AIR || 
                                                            grid[y+1][x+1].type == CELL_TYPE_WATER));
                    
                    // Choose direction based on scan direction or random if both possible
                    if(canMoveLeft && canMoveRight) {
                        int direction = processRightToLeft ? -1 : 1;
                        if(GetRandomValue(0, 100) < 50) direction *= -1;  // 50% chance to reverse
                        
                        if(direction == -1) {
                            // Transfer moisture if falling thru water
                            if(*moisture < 100 && grid[y+1][x-1].type == CELL_TYPE_WATER) {
                                AbsorbMoisture(&grid[y+1][x-1].moisture, moisture);
                            }
                            MoveCell(x, y, x-1, y+1);
                        } else {
                            // Transfer moisture if falling thru water
                            if(*moisture < 100 && grid[y+1][x+1].type == CELL_TYPE_WATER) {
                                AbsorbMoisture(&grid[y+1][x+1].moisture, moisture);
                            }
                            MoveCell(x, y, x+1, y+1);
                        }
                        grid[y][x].is_falling = true;
                    } 
                    else if(canMoveLeft) {
                        // Transfer moisture if falling thru water
                        if(*moisture < 100 && grid[y+1][x-1].type == CELL_TYPE_WATER) {
                            AbsorbMoisture(&grid[y+1][x-1].moisture, moisture);
                        }
                        MoveCell(x, y, x-1, y+1);
                        grid[y][x].is_falling = true;
                    }
                    else if(canMoveRight) {
                        // Transfer moisture if falling thru water
                        if(*moisture < 100 && grid[y+1][x+1].type == CELL_TYPE_WATER) {
                            AbsorbMoisture(&grid[y+1][x+1].moisture, moisture);
                        }
                        MoveCell(x, y, x+1, y+1);
                        grid[y][x].is_falling = true;
                    }
                }
            }
        }
    }
}

// Update water physics
void UpdateWater(void) {
    bool processRightToLeft = GetRandomValue(0, 1);
    
    // Process water from bottom to top for falling mechanics
    for(int y = GRID_HEIGHT - 1; y >= 0; y--) {
        // Alternate direction for each row
        processRightToLeft = !processRightToLeft;
        
        int startX, endX, stepX;
        if(processRightToLeft) {
            startX = GRID_WIDTH - 1;
            endX = -1;
            stepX = -1;
        } else {
            startX = 0;
            endX = GRID_WIDTH;
            stepX = 1;
        }
        
        for(int x = startX; x != endX; x += stepX) {
            if(grid[y][x].type == CELL_TYPE_WATER) {
                bool hasMoved = false;
                grid[y][x].is_falling = false;
                
                bool isAtBottomEdge = (y == GRID_HEIGHT - 1);
                
                // Check for moisture absorption
                if(grid[y][x].moisture < 100) {
                    if(!isAtBottomEdge && grid[y+1][x].type == CELL_TYPE_AIR) {
                        int canabsorb = grid[y+1][x].moisture - (100 - grid[y][x].moisture);
                        int leftover = grid[y+1][x].moisture - canabsorb;
                        grid[y][x].moisture += canabsorb;
                        grid[y+1][x].moisture = leftover;
                    }
                }
                
                // Count neighboring water cells for cohesion
                int waterNeighbors = 0;
                
                // Check horizontal neighbors
                for(int i = x-1; i >= 0 && i >= x-3; i--) {
                    if(grid[y][i].type == CELL_TYPE_WATER) {
                        waterNeighbors++;
                    } else {
                        break;
                    }
                }
                
                for(int i = x+1; i < GRID_WIDTH && i <= x+3; i++) {
                    if(grid[y][i].type == CELL_TYPE_WATER) {
                        waterNeighbors++;
                    } else {
                        break;
                    }
                }
                
                // Check vertical neighbors
                if(y > 0 && grid[y-1][x].type == CELL_TYPE_WATER) {
                    waterNeighbors += 2;
                }
                
                if(y < GRID_HEIGHT-1 && grid[y+1][x].type == CELL_TYPE_WATER) {
                    waterNeighbors += 2;
                }
                
                // Check if water has many neighbors (high cohesion)
                bool hasHighCohesion = (waterNeighbors >= 3);
                
                // Check if water can fall
                bool canFallDown = (!isAtBottomEdge && grid[y+1][x].type == CELL_TYPE_AIR);
                bool canFallDiagonalLeft = (!isAtBottomEdge && x > 0 && grid[y+1][x-1].type == CELL_TYPE_AIR);
                bool canFallDiagonalRight = (!isAtBottomEdge && x < GRID_WIDTH-1 && grid[y+1][x+1].type == CELL_TYPE_AIR);
                bool canFallAnyDirection = canFallDown || canFallDiagonalLeft || canFallDiagonalRight;
                
                // If water can fall, apply cohesion rules for falling
                if(canFallAnyDirection) {
                    if(canFallDown) {
                        // All water can fall straight down
                        MoveCell(x, y, x, y+1);
                        hasMoved = true;
                    } 
                    // Only water with low cohesion can fall diagonally
                    else if(!hasHighCohesion) {
                        if(canFallDiagonalLeft && canFallDiagonalRight) {
                            int direction = (GetRandomValue(0, 100) < 50) ? -1 : 1;
                            MoveCell(x, y, x+direction, y+1);
                            hasMoved = true;
                        } else if(canFallDiagonalLeft) {
                            MoveCell(x, y, x-1, y+1);
                            hasMoved = true;
                        } else if(canFallDiagonalRight) {
                            MoveCell(x, y, x+1, y+1);
                            hasMoved = true;
                        }
                    }
                }
                // If water cannot fall, it tries to spread horizontally
                else {
                    bool canFlowLeft = (x > 0 && grid[y][x-1].type == CELL_TYPE_AIR);
                    bool canFlowRight = (x < GRID_WIDTH-1 && grid[y][x+1].type == CELL_TYPE_AIR);
                    
                    // Horizontal spread is influenced by cohesion but still possible
                    // More cohesion = lower chance to spread
                    int spreadChance = hasHighCohesion ? 30 : 70;
                    
                    if(GetRandomValue(0, 100) < spreadChance) {
                        if(canFlowLeft && canFlowRight) {
                            int leftBias = processRightToLeft ? 60 : 40;
                            int flowDir = (GetRandomValue(0, 100) < leftBias) ? -1 : 1;
                            MoveCell(x, y, x+flowDir, y);
                            hasMoved = true;
                        } else if(canFlowLeft) {
                            MoveCell(x, y, x-1, y);
                            hasMoved = true;
                        } else if(canFlowRight) {
                            MoveCell(x, y, x+1, y);
                            hasMoved = true;
                        }
                    }
                }
                
                // Update falling state based on movement
                grid[y][x].is_falling = hasMoved && canFallAnyDirection;
            }
        }
    }
}
