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
    
    // Only start vapor processing after a few frames to allow initialization to complete
    if(updateCount < 5) {
        UpdateWater();
        UpdateSoil();
        // Skip vapor updates for first few frames
    } else {
        UpdateWater();
        UpdateSoil();
        UpdateVapor(); // Run vapor updates after initialization is stable
    }
    
    // Handle moisture transfer at the end of each cycle
    TransferMoisture();
}

// Update soil physics
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
                
                // Track soil moisture
                int* moisture = &grid[y][x].moisture;

                // FALLING MECHANICS: explicitly prioritize falling through vapor
                if(y < GRID_HEIGHT - 1) {
                    // First priority: Fall through vapor regardless of moisture levels
                    if(grid[y+1][x].type == CELL_TYPE_VAPOR) {
                        // Mark as falling and move, let MoveCell handle the moisture correctly
                        grid[y][x].is_falling = true;
                        MoveCell(x, y, x, y+1);
                        continue;
                    }
                    
                    // Second priority: Check for empty space or water
                    if(grid[y+1][x].type == CELL_TYPE_AIR || grid[y+1][x].type == CELL_TYPE_WATER) {
                        grid[y][x].is_falling = true;
                        MoveCell(x, y, x, y+1);
                        continue;
                    }
                    
                    // For diagonal sliding, also prioritize vapor
                    bool canSlideLeftVapor = (x > 0 && grid[y+1][x-1].type == CELL_TYPE_VAPOR);
                    bool canSlideRightVapor = (x < GRID_WIDTH-1 && grid[y+1][x+1].type == CELL_TYPE_VAPOR);
                    
                    // First check vapor diagonally
                    if(canSlideLeftVapor && canSlideRightVapor) {
                        int bias = processRightToLeft ? 40 : 60;
                        int slideDir = (GetRandomValue(0, 100) < bias) ? -1 : 1;
                        grid[y][x].is_falling = true;
                        MoveCell(x, y, x+slideDir, y+1);
                        continue;
                    } else if(canSlideLeftVapor) {
                        grid[y][x].is_falling = true;
                        MoveCell(x, y, x-1, y+1);
                        continue;
                    } else if(canSlideRightVapor) {
                        grid[y][x].is_falling = true;
                        MoveCell(x, y, x+1, y+1);
                        continue;
                    }
                    
                    // Then check other cells (empty, water) diagonally
                    bool canSlideLeft = (x > 0 && (grid[y+1][x-1].type == CELL_TYPE_AIR || 
                                                  grid[y+1][x-1].type == CELL_TYPE_WATER || 
                                                  grid[y+1][x-1].type == CELL_TYPE_VAPOR));
                    bool canSlideRight = (x < GRID_WIDTH-1 && (grid[y+1][x+1].type == CELL_TYPE_AIR || 
                                                              grid[y+1][x+1].type == CELL_TYPE_WATER || 
                                                              grid[y+1][x+1].type == CELL_TYPE_VAPOR));
                    
                    if(canSlideLeft && canSlideRight) {
                        // Choose random direction, but slightly favor the current processing direction
                        int bias = processRightToLeft ? 40 : 60; // 40/60 bias instead of 50/50
                        int slideDir = (GetRandomValue(0, 100) < bias) ? -1 : 1;
                        grid[y][x].is_falling = true;
                        MoveCell(x, y, x+slideDir, y+1);
                    } else if(canSlideLeft) {
                        grid[y][x].is_falling = true;
                        MoveCell(x, y, x-1, y+1);
                    } else if(canSlideRight) {
                        grid[y][x].is_falling = true;
                        MoveCell(x, y, x+1, y+1);
                    }
                }

                // Update soil color based on moisture
                float intensityPct = (float)*moisture / 100.0f;
                grid[y][x].baseColor = (Color){
                    127 - (intensityPct * 51),
                    106 - (intensityPct * 43),
                    79 - (intensityPct * 32),
                    255
                };

                // Transfer tiny amounts to ambient vapor if needed
                if(grid[y][x].moisture > 20) {
                    // Try to find nearby ambient vapor to add moisture to
                    bool transferred = false;
                    for(int dy = -1; dy <= 1 && !transferred; dy++) {
                        for(int dx = -1; dy <= 1 && !transferred; dx++) {
                            if(dx == 0 && dy == 0) continue;
                            
                            int nx = x + dx;
                            int ny = y + dy;
                            
                            if(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT && 
                               grid[ny][nx].type == CELL_TYPE_VAPOR) {
                                // Transfer a tiny amount of moisture
                                int transferAmount = 1;
                                if(grid[y][x].moisture - transferAmount < 20) {
                                    transferAmount = grid[y][x].moisture - 20;
                                }
                                
                                if(transferAmount > 0) {
                                    grid[y][x].moisture -= transferAmount;
                                    grid[ny][nx].moisture += transferAmount;
                                    transferred = true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

// Update water physics
void UpdateWater(void) {
    // ... [Implementation from the original code]
    // For brevity, I'm not including the full implementation here, but you would copy the entire UpdateWater function
}

// Update vapor physics
void UpdateVapor(void) {
    // ... [Implementation from the original code]
    // For brevity, I'm not including the full implementation here, but you would copy the entire UpdateVapor function
}

// Handle moisture transfer between cells
void TransferMoisture(void) {
    // ... [Implementation from the original code]
    // For brevity, I'm not including the full implementation here, but you would copy the entire TransferMoisture function
}

// Flood fill algorithm for ceiling water clusters
void FloodFillCeilingCluster(int x, int y, int clusterID, int** clusterIDs) {
    // Check bounds
    if(x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) return;
    
    // Only process water cells that haven't been assigned a cluster yet
    if(grid[y][x].type != CELL_TYPE_WATER || clusterIDs[y][x] != -1) return;
    
    // Must be ceiling water (at top or has water above it)
    if(y > 0 && grid[y-1][x].type != CELL_TYPE_WATER && y != 0) return;
    
    // Assign this cell to the cluster
    clusterIDs[y][x] = clusterID;
    
    // Recursively process neighboring cells (only horizontal and below)
    FloodFillCeilingCluster(x-1, y, clusterID, clusterIDs);  // Left
    FloodFillCeilingCluster(x+1, y, clusterID, clusterIDs);  // Right
    FloodFillCeilingCluster(x, y+1, clusterID, clusterIDs);  // Below
    FloodFillCeilingCluster(x-1, y+1, clusterID, clusterIDs); // Diag below left
    FloodFillCeilingCluster(x+1, y+1, clusterID, clusterIDs); // Diag below right
}
