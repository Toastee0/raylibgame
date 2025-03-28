#include "simulation.h"
#include "grid.h"
#include "cell_types.h"
#include "cell_actions.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>


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
    
    // Update all cell types in the right order
  //  UpdateSoil();         // Soil falls
    UpdateWater();        // Water flows
   // UpdateEvaporation();  // Water evaporates based on temperature
    UpdateAir();          // Moist air rises, and clouds form in cool regions
    
    // Other update functions...
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
              //  grid[y][x].is_falling = false;
              //  bool hasMoved = false;
                
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
                        if (grid[y][x].type == CELL_TYPE_AIR) {
                            //check if we are already full of moisture.
                            if(*moisture < 100) {
                                // Transfer moisture from soil to air
                                int canabsorb = (100 - *moisture);
                                int transferamount = grid[y+1][x].moisture - canabsorb;
                                grid[y][x].moisture += transferamount;
                                grid[y+1][x].moisture -= transferamount;
                            }
                        }
                        MoveCell(x, y, x, y+1);
                        grid[y+1][x].is_falling = true;
                        
                        continue;  // Skip further checks, we've moved
                    }
                }

                // Check if soil can fall diagonally
                if(y < GRID_HEIGHT - 1) {
                    bool canMoveLeft = (x > 0 && (grid[y+1][x-1].type == CELL_TYPE_AIR || 
                                                 grid[y+1][x-1].type == CELL_TYPE_WATER));
                    bool canMoveRight = (x < GRID_WIDTH-1 && (grid[y+1][x+1].type == CELL_TYPE_AIR || 
                                                            grid[y+1][x+1].type == CELL_TYPE_WATER));
                    
                    // Choose direction based on scan direction or random if both possible
                    if(canMoveLeft && canMoveRight) {
                        int direction = processRightToLeft ? -1 : 1;
                        if(GetRandomValue(0, 100) < 50) direction *= -1;  // 50% chance to reverse
                        
                        if(direction == -1) {
                          
                            MoveCell(x, y, x+1, y+1);
                        }
                        grid[y][x].is_falling = true;
                    } 
                    else if(canMoveLeft) {
                       
                       
                        MoveCell(x, y, x-1, y+1);
                        grid[y][x].is_falling = true;
                    }
                    else if(canMoveRight) {
                       
                       
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
                if(grid[y][x].moisture < 1000) {
                    if(!isAtBottomEdge && grid[y+1][x].type == CELL_TYPE_AIR) {
                        int canabsorb =  (100 - grid[y][x].moisture);
                        int leftover = grid[y+1][x].moisture - canabsorb;
                        grid[y][x].moisture += leftover;
                        grid[y+1][x].moisture -= leftover;
                        MoveCell(x, y, x, y+1);
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

// Update air physics - makes moist air rise
void UpdateAir(void) {
    // Use alternating row processing like in soil and water
    bool processRightToLeft = GetRandomValue(0, 1);
    
    for(int y = 1; y < GRID_HEIGHT - 1; y++) {
        // Alternate direction for each row
        processRightToLeft = !processRightToLeft;
        
        // Set iteration direction based on processRightToLeft
        int startX, endX, stepX;
        if(processRightToLeft) {
            startX = GRID_WIDTH - 2;  // Skip border
            endX = 0;
            stepX = -1;
        } else {
            startX = 1;  // Skip border
            endX = GRID_WIDTH - 1;
            stepX = 1;
        }
        
        for(int x = startX; x != endX; x += stepX) {
            if(grid[y][x].type != CELL_TYPE_AIR || grid[y][x].type == CELL_TYPE_BORDER) {
                continue;
            }
            
            // get distance to next solid or border cell above
            int distanceToSolid = 0;
            for(int i = y-1; i >= 0; i--) {
                if(grid[i][x].type == CELL_TYPE_BORDER || grid[i][x].type == CELL_TYPE_ROCK) {
                    break;
                }
                distanceToSolid++;
            }

            // if we are 3-9 cells away from a solid cell and we have 100 moisture we can form a droplet
            if(distanceToSolid > 3 && distanceToSolid < 9 && grid[y][x].moisture >= 100) {
                grid[y][x].type = CELL_TYPE_WATER;
                grid[y][x].baseColor = BLUE; // Set color to blue for water
                
                //gather moisture from any adjacent air cells
                for(int dy = -1; dy <= 1; dy++) {
                    for(int dx = -1; dx <= 1; dx++) {
                        if((dx == 0 && dy == 0) || 
                           y+dy < 0 || y+dy >= GRID_HEIGHT ||
                           x+dx < 0 || x+dx >= GRID_WIDTH) {
                            continue; // FIXED: Added continue statement
                        }
                            
                        if(grid[y+dy][x+dx].type == CELL_TYPE_AIR && grid[y+dy][x+dx].moisture > 0) {
                            int availableSpace = 1000 - grid[y][x].moisture;
                            int takenAmount = grid[y+dy][x+dx].moisture;
                            if(availableSpace > 0) {
                                grid[y][x].moisture += takenAmount;
                                grid[y+dy][x+dx].moisture -= takenAmount;
                            }
                        }
                    }
                }
                
                continue; // FIXED: Skip rest of processing since we're now water
            } // FIXED: Properly closed the water formation if-block

            // Rising behavior  
            if(y > 0 && grid[y-1][x].type == CELL_TYPE_AIR) {
                if(grid[y-1][x].moisture < grid[y][x].moisture) {
                    MoveCell(x, y, x, y-1);
                    continue;
                }
            } 
           
            // Horizontal movement - randomized direction
            bool moveLeft = (GetRandomValue(0, 1) == 0);
            if(moveLeft) {
                if(x > 0 && grid[y][x-1].type == CELL_TYPE_AIR) {
                    MoveCell(x, y, x-1, y);
                    continue; // Skip further processing
                }
            } else {
                if(x < GRID_WIDTH-1 && grid[y][x+1].type == CELL_TYPE_AIR) {
                    MoveCell(x, y, x+1, y);
                    continue; // Skip further processing
                }
            }
            
            // Diagonal movement - randomize left/right choice
            bool canMoveUpLeft = (y > 0 && x > 0 && 
                                grid[y-1][x-1].type == CELL_TYPE_AIR &&
                                grid[y-1][x-1].moisture < grid[y][x].moisture);
                                
            bool canMoveUpRight = (y > 0 && x < GRID_WIDTH-1 && 
                                 grid[y-1][x+1].type == CELL_TYPE_AIR &&
                                 grid[y-1][x+1].moisture < grid[y][x].moisture);
            
            if(canMoveUpLeft && canMoveUpRight) {
                // Both diagonals available - choose randomly
                if(GetRandomValue(0, 1) == 0) {
                    MoveCell(x, y, x-1, y-1);
                } else {
                    MoveCell(x, y, x+1, y-1);
                }
            } else if(canMoveUpLeft) {
                MoveCell(x, y, x-1, y-1);
            } else if(canMoveUpRight) {
                MoveCell(x, y, x+1, y-1);
            }
            
            // Update air color
            UpdateAirColor(x, y);
        }
    }
}

// Helper function to update air cell color based on moisture
void UpdateAirColor(int x, int y) {
    if(grid[y][x].type == CELL_TYPE_AIR) {
        if(grid[y][x].moisture > 75) {
            int brightness = (grid[y][x].moisture - 75) * (255 / 25);
            grid[y][x].baseColor = (Color){brightness, brightness, brightness, 255};
        } else {
            grid[y][x].baseColor = BLACK;  // Invisible air
        }
    }
}



// Update evaporation considering temperature
void UpdateEvaporation(void) {
    for(int y = 0; y < GRID_HEIGHT; y++) {
        for(int x = 0; x < GRID_WIDTH; x++) {
            if(grid[y][x].type == CELL_TYPE_WATER && grid[y][x].moisture > 30) {
                // Evaporation rate increases with temperature
                float evapRate = 0.5f + (grid[y][x].temperature - 10.0f) * 0.05f;
                if(evapRate < 0.1f) evapRate = 0.1f;
                
                // Basic chance for evaporation
                if(GetRandomValue(0, 100) < evapRate * 100) {
                    // Look for air cells to transfer moisture to
                    for(int dy = -1; dy <= 1; dy++) {
                        for(int dx = -1; dx <= 1; dx++) {
                            if((dx == 0 && dy == 0) || 
                               y+dy < 0 || y+dy >= GRID_HEIGHT ||
                               x+dx < 0 || x+dx >= GRID_WIDTH)
                                continue;
                                
                            if(grid[y+dy][x+dx].type == CELL_TYPE_AIR && grid[y+dy][x+dx].moisture < 95) {
                                int evapAmount = 1 + GetRandomValue(0, 2);
                                
                                // Adjust based on temperature difference
                                float tempDiff = grid[y+dy][x+dx].temperature - grid[y][x].temperature;
                                if(tempDiff < 0) evapAmount = (int)(evapAmount * (1.0f + tempDiff * 0.1f));
                                
                                if(evapAmount < 1) evapAmount = 1;
                                
                                if(grid[y][x].moisture - evapAmount >= 20) {
                                    grid[y][x].moisture -= evapAmount;
                                    grid[y+dy][x+dx].moisture += evapAmount;
                                    UpdateAirColor(x+dx, y+dy);
                                }
                                
                                break;  // Only evaporate to one cell per update
                            }
                        }
                    }
                }
            }
        }
    }
}

// Initialize temperature gradient across the grid
void InitializeTemperature(void) {
    const float baseTemp = 18.0f;     // Bottom temperature in Celsius
    const float topTemp = 5.0f;       // Top temperature in Celsius
    const float tempRange = baseTemp - topTemp;
    
    for(int y = 0; y < GRID_HEIGHT; y++) {
        // Calculate temperature based on y position (cooler at top)
        float tempAtHeight = baseTemp - (tempRange * (float)y / GRID_HEIGHT);
        
        for(int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x].temperature = tempAtHeight;
        }
    }
}
