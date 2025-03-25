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
static void UpdateWater(void) {
    // Randomly decide initial direction for this cycle
    bool processRightToLeft = GetRandomValue(0, 1);
    
    // First, calculate ceiling water clusters
    int** ceilingClusterIDs = (int**)malloc(GRID_HEIGHT * sizeof(int*));
    for(int i = 0; i < GRID_HEIGHT; i++) {
        ceilingClusterIDs[i] = (int*)malloc(GRID_WIDTH * sizeof(int));
        for(int j = 0; j < GRID_WIDTH; j++) {
            ceilingClusterIDs[i][j] = -1; // -1 means not part of cluster
        }
    }
    
    // Assign cluster IDs using flood fill
    int nextClusterID = 0;
    for(int y = 0; y < GRID_HEIGHT; y++) {
        for(int x = 0; x < GRID_WIDTH; x++) {
            // If water and either at ceiling (y=0) or has water above
            if(grid[y][x].type == 2 && (y == 0 || (y > 0 && grid[y-1][x].type == 2))) {
                if(ceilingClusterIDs[y][x] == -1) {
                    // Start a new cluster with flood fill
                    FloodFillCeilingCluster(x, y, nextClusterID, ceilingClusterIDs);
                    nextClusterID++;
                }
            }
        }
    }
    
    // Count cells in each cluster
    int* clusterSizes = (int*)calloc(nextClusterID, sizeof(int));
    for(int y = 0; y < GRID_HEIGHT; y++) {
        for(int x = 0; x < GRID_WIDTH; x++) {
            if(ceilingClusterIDs[y][x] >= 0) {
                clusterSizes[ceilingClusterIDs[y][x]]++;
            }
        }
    }
    
    // Process water from bottom to top for falling mechanics
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
            if(grid[y][x].type == 2) {  // If it's water
                bool hasMoved = false;
                
                // Reset falling state before movement logic
                grid[y][x].is_falling = false;
                
                // Check if this is ceiling water (part of a cluster)
                if(ceilingClusterIDs[y][x] >= 0) {
                    int clusterID = ceilingClusterIDs[y][x];
                    int clusterSize = clusterSizes[clusterID];
                    
                    // FIXED: Nucleation effect - Gentler, more controlled moisture attraction
                    // Limit attraction radius to prevent affecting distant cells
                    int attractRadius = 1 + (int)(sqrtf(clusterSize) * 0.5f);
                    if(attractRadius > 3) attractRadius = 3;  // Reduced maximum radius
                    
                    // Limit the total amount of moisture a cluster can attract per frame
                    float maxTotalAttraction = 0.05f * sqrtf(clusterSize);
                    float totalAttracted = 0.0f;
                    
                    // Only attract vapor from above and sides, not below (where sand might be)
                    for(int dy = -attractRadius; dy <= 0; dy++) {
                        for(int dx = -attractRadius; dx <= attractRadius; dx++) {
                            // Skip if accumulated too much already
                            if(totalAttracted >= maxTotalAttraction) continue;
                            
                            int nx = x + dx, ny = y + dy;
                            
                            // STRICT type checking - ONLY affect vapor cells
                            if(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT && 
                               grid[ny][nx].type == 4) {
                                
                                // Calculate distance factor
                                float distance = sqrtf(dx*dx + dy*dy);
                                if(distance <= attractRadius) {
                                    // Reduced transfer factor (much gentler)
                                    float transferFactor = (1.0f - distance/attractRadius) * 0.05f;
                                    
                                    // Cap transfer based on moisture
                                    float transferAmount = grid[ny][nx].moisture * transferFactor;
                                    transferAmount = fmin(transferAmount, 0.05f); // Hard cap per cell
                                    
                                    // Leave vapor with a minimum moisture
                                    if(grid[ny][nx].moisture - transferAmount < 0.02f) {
                                        transferAmount = grid[ny][nx].moisture - 0.02f;
                                    }
                                    
                                    // Skip minimal transfers
                                    if(transferAmount < 0.001f) continue;
                                    
                                    // Track total attraction to respect cap
                                    totalAttracted += transferAmount;
                                    
                                    // Safety check - don't exceed total limit
                                    if(totalAttracted > maxTotalAttraction) {
                                        transferAmount -= (totalAttracted - maxTotalAttraction);
                                        totalAttracted = maxTotalAttraction;
                                    }
                                    
                                    if(transferAmount > 0) {
                                        grid[y][x].moisture += transferAmount;
                                        grid[ny][nx].moisture -= transferAmount;
                                        
                                        // Update colors
                                        float intensity = grid[y][x].moisture;
                                        grid[y][x].baseColor = (Color){
                                            0 + (int)(200 * (1.0f - intensity)),
                                            120 + (int)(135 * (1.0f - intensity)),
                                            255,
                                            255
                                        };
                                        
                                        if(grid[ny][nx].moisture < 0.2f) {
                                            grid[ny][nx].baseColor = BLACK;
                                        } else {
                                            int brightness = 128 + (int)(127 * grid[ny][nx].moisture);
                                            grid[ny][nx].baseColor = (Color){
                                                brightness, brightness, brightness, 255
                                            };
                                        }
                                    }
                                }
                            }
                        }
                    }
                    
                    // Ceiling water surface tension: larger clusters hold longer
                    float dropChance = 0.01f + (10.0f / (clusterSize + 10.0f));
                    
                    // Water only stays attached until the cluster gets very large (15+)
                    if(clusterSize > 15) {
                        dropChance += (clusterSize - 15) * 0.05f;
                    }
                    
                    // Check if this drop falls
                    if(GetRandomValue(0, 100) < dropChance * 100) {
                        // Standard falling logic
                        if(y < GRID_HEIGHT - 1) {  // If not at bottom
                            // Special check for vapor below (water falls through vapor)
                            if(grid[y+1][x].type == 4) {
                                MoveCell(x, y, x, y+1);  // Will trigger the swap in MoveCell
                                hasMoved = true;
                                continue;
                            }
                            
                            // Also check for vapor diagonally below
                            bool vaporBelowLeft = (x > 0 && grid[y+1][x-1].type == 4);
                            bool vaporBelowRight = (x < GRID_WIDTH-1 && grid[y+1][x+1].type == 4);
                            
                            if(vaporBelowLeft && vaporBelowRight) {
                                // Choose random direction
                                int direction = (GetRandomValue(0, 1) == 0) ? -1 : 1;
                                MoveCell(x, y, x+direction, y+1);
                                hasMoved = true;
                                continue;
                            } else if(vaporBelowLeft) {
                                MoveCell(x, y, x-1, y+1);
                                hasMoved = true;
                                continue;
                            } else if(vaporBelowRight) {
                                MoveCell(x, y, x+1, y+1);
                                hasMoved = true;
                                continue;
                            }
                            
                            // Try to fall straight down first
                            if(grid[y+1][x].type == 0) {
                                MoveCell(x, y, x, y+1);
                                hasMoved = true;
                                continue;
                            }
                            
                            // Count neighboring water cells for surface tension
                            int waterNeighbors = 0;
                            for(int dy = -1; dy <= 1; dy++) {
                                for(int dx = -1; dx <= 1; dx++) {
                                    if(dx == 0 && dy == 0) continue;
                                    
                                    int nx = x + dx;
                                    int ny = y + dy;
                                    if(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT && 
                                       grid[ny][nx].type == 2) {
                                        waterNeighbors++;
                                    }
                                }
                            }
                            
                            // Apply surface tension logic: prefer staying in clusters
                            float moveChance = (waterNeighbors > 2) ? 0.3f : 1.0f;
                            
                            if(GetRandomValue(0, 100) < moveChance * 100) {
                                // Try to flow diagonally down if can't go straight down
                                bool canFlowDownLeft = (x > 0 && y < GRID_HEIGHT-1 && grid[y+1][x-1].type == 0);
                                bool canFlowDownRight = (x < GRID_WIDTH-1 && y < GRID_HEIGHT-1 && grid[y+1][x+1].type == 0);
                                
                                if(canFlowDownLeft && canFlowDownRight) {
                                    // Choose random direction with slight bias
                                    int direction = (GetRandomValue(0, 100) < 50) ? -1 : 1;
                                    MoveCell(x, y, x+direction, y+1);
                                    hasMoved = true;
                                } else if(canFlowDownLeft) {
                                    MoveCell(x, y, x-1, y+1);
                                    hasMoved = true;
                                } else if(canFlowDownRight) {
                                    MoveCell(x, y, x+1, y+1);
                                    hasMoved = true;
                                } else {
                                    // If can't fall, try to spread horizontally
                                    bool canFlowLeft = (x > 0 && grid[y][x-1].type == 0);
                                    bool canFlowRight = (x < GRID_WIDTH-1 && grid[y][x+1].type == 0);
                                    
                                    if(canFlowLeft && canFlowRight) {
                                        // Slightly favor the current processing direction
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
                        }
                    } else {
                        // This water stays attached to ceiling
                        hasMoved = true; // Mark as "moved" to prevent evaporation
                        continue;
                    }
                }
                
                // Normal water movement logic for non-ceiling water
                if(y < GRID_HEIGHT - 1) {  // If not at bottom
                    // Special check for vapor below (water falls through vapor)
                    if(grid[y+1][x].type == 4) {
                        MoveCell(x, y, x, y+1);  // Will trigger the swap in MoveCell
                        hasMoved = true;
                        continue;
                    }
                    
                    // Also check for vapor diagonally below
                    bool vaporBelowLeft = (x > 0 && grid[y+1][x-1].type == 4);
                    bool vaporBelowRight = (x < GRID_WIDTH-1 && grid[y+1][x+1].type == 4);
                    
                    if(vaporBelowLeft && vaporBelowRight) {
                        // Choose random direction
                        int direction = (GetRandomValue(0, 1) == 0) ? -1 : 1;
                        MoveCell(x, y, x+direction, y+1);
                        hasMoved = true;
                        continue;
                    } else if(vaporBelowLeft) {
                        MoveCell(x, y, x-1, y+1);
                        hasMoved = true;
                        continue;
                    } else if(vaporBelowRight) {
                        MoveCell(x, y, x+1, y+1);
                        hasMoved = true;
                        continue;
                    }
                    
                    // Try to fall straight down first
                    if(grid[y+1][x].type == 0) {
                        MoveCell(x, y, x, y+1);
                        hasMoved = true;
                        continue;
                    }
                    
                    // Count neighboring water cells for surface tension
                    int waterNeighbors = 0;
                    for(int dy = -1; dy <= 1; dy++) {
                        for(int dx = -1; dx <= 1; dx++) {
                            if(dx == 0 && dy == 0) continue;
                            
                            int nx = x + dx;
                            int ny = y + dy;
                            if(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT && 
                               grid[ny][nx].type == 2) {
                                waterNeighbors++;
                            }
                        }
                    }
                    
                    // Apply surface tension logic: prefer staying in clusters
                    float moveChance = (waterNeighbors > 2) ? 0.3f : 1.0f;

                    if(GetRandomValue(0, 100) < moveChance * 100) {
                        // Try to flow diagonally down if can't go straight down
                        bool canFlowDownLeft = (x > 0 && y < GRID_HEIGHT-1 && grid[y+1][x-1].type == 0);
                        bool canFlowDownRight = (x < GRID_WIDTH-1 && y < GRID_HEIGHT-1 && grid[y+1][x+1].type == 0);
                        
                        if(canFlowDownLeft && canFlowDownRight) {
                            // Choose random direction with slight bias
                            int direction = (GetRandomValue(0, 100) < 50) ? -1 : 1;
                            MoveCell(x, y, x+direction, y+1);
                            hasMoved = true;
                        } else if(canFlowDownLeft) {
                            MoveCell(x, y, x-1, y+1);
                            hasMoved = true;
                        } else if(canFlowDownRight) {
                            MoveCell(x, y, x+1, y+1);
                            hasMoved = true;
                        } else {
                            // If can't fall, try to spread horizontally
                            bool canFlowLeft = (x > 0 && grid[y][x-1].type == 0);
                            bool canFlowRight = (x < GRID_WIDTH-1 && grid[y][x+1].type == 0);
                            
                            if(canFlowLeft && canFlowRight) {
                                // Slightly favor the current processing direction
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
                }
                
                // When water moves in any way, mark it as falling
                if(hasMoved) {
                    grid[y][x].is_falling = true;
                }
                
                // Use both volume and is_falling to track movement state
                if(!hasMoved && grid[y][x].type == 2) {  // Double-check it's still water
                    grid[y][x].volume = 0;  // Use volume field to flag that water didn't move
                    grid[y][x].is_falling = false;
                } else {
                    grid[y][x].volume = 1;  // Flag that water has moved
                }
            }
        }
    }
    
    // Cleanup the cluster tracking arrays
    for(int i = 0; i < GRID_HEIGHT; i++) {
        free(ceilingClusterIDs[i]);
    }
    free(ceilingClusterIDs);
    free(clusterSizes);
    
    // After general water movement, handle horizontal density diffusion
    // Process from left to right and right to left alternately to avoid bias
    for(int y = 0; y < GRID_HEIGHT; y++) {
        bool rowRightToLeft = GetRandomValue(0, 1);
        int startX = rowRightToLeft ? GRID_WIDTH - 2 : 1;
        int endX = rowRightToLeft ? 0 : GRID_WIDTH - 1;
        int stepX = rowRightToLeft ? -1 : 1;
        
        for(int x = startX; x != endX; x += stepX) {
            if(grid[y][x].type == 2) {  // If it's water
                // Check horizontally adjacent water cells for density differences
                if(x > 0 && grid[y][x-1].type == 2) {
                    // Average densities for adjacent water cells
                    int avgMoisture = (grid[y][x].moisture + grid[y][x-1].moisture) / 2;
                    // Small equalization step (don't fully equalize)
                    grid[y][x].moisture = (grid[y][x].moisture * 9 + avgMoisture) / 10;
                    grid[y][x-1].moisture = (grid[y][x-1].moisture * 9 + avgMoisture) / 10;
                }
                if(x < GRID_WIDTH-1 && grid[y][x+1].type == 2) {
                    // Average densities for adjacent water cells
                    int avgMoisture = (grid[y][x].moisture + grid[y][x+1].moisture) / 2;
                    // Small equalization step
                    grid[y][x].moisture = (grid[y][x].moisture * 9 + avgMoisture) / 10;
                    grid[y][x+1].moisture = (grid[y][x+1].moisture * 9 + avgMoisture) / 10;
                }
            }
        }
    }
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
