#include "raylib.h"
#include "updatecells.h"
#include "grid.h"
#include "cell_types.h"
#include "cell_defaults.h" 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// Direction arrays for easy iteration through neighbors
const int DIR_X[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
const int DIR_Y[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

// Base densities for different cell types
const float BASE_DENSITIES[6] = {
    1.0f,   // CELL_TYPE_AIR - lowest density
    20.0f,  // CELL_TYPE_SOIL - medium density
    10.0f,  // CELL_TYPE_WATER - lower than soil
    5.0f,   // CELL_TYPE_PLANT - lighter than soil
    30.0f,  // CELL_TYPE_ROCK - highest density
    15.0f   // CELL_TYPE_MOSS - between water and soil
};

// Function to get a random number between 0 and 99
int random_chance(void) {
    return rand() % 100;
}

// Function to calculate cell density based on type and composition
float calculate_density(GridCell* cell) {
    // Use the predefined base density or the cell's own density value if set
    float density = cell->density > 0 ? cell->density : BASE_DENSITIES[cell->type];
    
    // Water content increases density
    density += (cell->water / 100.0f) * 2.0f;
    
    return density;
}

// Function to swap two cells
void swap_cells(int x1, int y1, int x2, int y2) {
    GridCell temp = grid[y1][x1];

    grid[y1][x1] = grid[y2][x2];
    grid[y2][x2] = temp;
      // Mark both cells as updated
    grid[y1][x1].updated_this_frame = true;
    grid[y2][x2].updated_this_frame = true;
}

// Function to displace air and water with falling soil cells
// For the cell we are trying to move into, check if that cell has any adjacent cells of the same type (materials that behave as fluids: air/water)
// The pressure of the cell we are trying to move in is distributed to adjacent cells, then we move the falling soil cell into the target cell.
void push_cellvoid(int x1, int y1, int x2, int y2) {
    // Check if source and target are valid and if target is a fluid (air or water)
    if (x1 <= 0 || y1 <= 0 || x2 <= 0 || y2 <= 0 || 
        x1 >= GRID_WIDTH-1 || y1 >= GRID_HEIGHT-1 || 
        x2 >= GRID_WIDTH-1 || y2 >= GRID_HEIGHT-1) {
        return; // Invalid coordinates
    }
    
    if (grid[y2][x2].type != CELL_TYPE_AIR && grid[y2][x2].type != CELL_TYPE_WATER) {
        return; // Target is not a fluid we can displace
    }
    
    // Find adjacent cells of the same fluid type as the target
    int count = 0;
    int adj_x[4] = {0, 0, -1, 1}; // Up, Down, Left, Right
    int adj_y[4] = {-1, 1, 0, 0};
    int valid_adj_x[4] = {0};
    int valid_adj_y[4] = {0};
    
    for (int i = 0; i < 4; i++) {
        int nx = x2 + adj_x[i];
        int ny = y2 + adj_y[i];
        
        if (nx > 0 && nx < GRID_WIDTH-1 && ny > 0 && ny < GRID_HEIGHT-1 && 
            grid[ny][nx].type == grid[y2][x2].type && 
            !grid[ny][nx].updated_this_frame) {
            valid_adj_x[count] = nx;
            valid_adj_y[count] = ny;
            count++;
        }
    }
    
    // If we found adjacent fluid cells of the same type
    if (count > 0) {
        // Safety check to prevent overflow - cap pressure at INT_MAX/2
        int safe_pressure = grid[y2][x2].pressure;
        if (safe_pressure > 1000000) safe_pressure = 1000000; // Reasonable upper limit
        
        // Instead of picking a random cell, distribute the pressure and contents to ALL valid adjacent cells
        int pressure_per_cell = (safe_pressure + 1) / count;
        int oxygen_per_cell = grid[y2][x2].oxygen / count;
        int water_per_cell = grid[y2][x2].water / count;
        int mineral_per_cell = grid[y2][x2].mineral / count;
        
        // Remainder values (if division isn't even)
        int pressure_remainder = (safe_pressure + 1) % count;
        int oxygen_remainder = grid[y2][x2].oxygen % count;
        int water_remainder = grid[y2][x2].water % count;
        int mineral_remainder = grid[y2][x2].mineral % count;
        
        for (int i = 0; i < count; i++) {
            int nx = valid_adj_x[i];
            int ny = valid_adj_y[i];
            
            // Safety check before adding to prevent overflow
            if (grid[ny][nx].pressure > 1000000 - pressure_per_cell) {
                grid[ny][nx].pressure = 1000000;
            } else {
                grid[ny][nx].pressure += pressure_per_cell;
            }
            
            // Add the remaining properties with safety limits
            if (grid[ny][nx].oxygen <= 1000 - oxygen_per_cell)
                grid[ny][nx].oxygen += oxygen_per_cell;
            
            if (grid[ny][nx].water <= 1000 - water_per_cell)
                grid[ny][nx].water += water_per_cell;
                
            if (grid[ny][nx].mineral <= 1000 - mineral_per_cell)
                grid[ny][nx].mineral += mineral_per_cell;
            
            // Add remainders to the first cell to avoid losing any value
            if (i == 0) {
                if (grid[ny][nx].pressure <= 1000000 - pressure_remainder)
                    grid[ny][nx].pressure += pressure_remainder;
                    
                if (grid[ny][nx].oxygen <= 1000 - oxygen_remainder)
                    grid[ny][nx].oxygen += oxygen_remainder;
                    
                if (grid[ny][nx].water <= 1000 - water_remainder)
                    grid[ny][nx].water += water_remainder;
                    
                if (grid[ny][nx].mineral <= 1000 - mineral_remainder)
                    grid[ny][nx].mineral += mineral_remainder;
            }
            
            // Mark the adjacent cell as updated this frame
            grid[ny][nx].updated_this_frame = true;
        }
        
        
        // Copy source to target
        grid[y2][x2] = grid[y1][x1];
        
        grid[y2][x2].updated_this_frame = true;
        grid[y2][x2].is_falling = true;
        
        // Mark the source cell as empty
        grid[y1][x1].type = CELL_TYPE_EMPTY;
        grid[y1][x1].empty = true;
        grid[y1][x1].updated_this_frame = true;
    }
}

// Air movement - air rises or moves horizontally
/*
air is defined as a cell that contains mostly oxygen, it can carry small amounts of water and minerals.
air with water in it is counted as less dense than air without water, and will rise. 
air that is 2 cells below a border cell, or other immovable cell like rock will be considered a candidated for water droplet formation.
border cells do not accept pressure, and cannot be moved into. border cells cannot be changed in any way.
air will fill adjacent cells that are flagged as empty as a priority, by donating pressure to them
*/
void update_air(void) {
    // Process from top to bottom to properly handle air movement
    for (int y = 1; y < GRID_HEIGHT - 1; y++) {
        for (int x = 1; x < GRID_WIDTH - 1; x++) {
            // Skip if already processed this frame
            if (grid[y][x].updated_this_frame || grid[y][x].type != CELL_TYPE_AIR)
                continue;
                
            // PRIORITY 1: Fill adjacent empty cells
            bool filled_empty = false;
            for (int i = 0; i < 8; i++) {
                int nx = x + DIR_X[i];
                int ny = y + DIR_Y[i];
                
                if (nx > 0 && nx < GRID_WIDTH - 1 && ny > 0 && ny < GRID_HEIGHT - 1 && 
                    grid[ny][nx].type == CELL_TYPE_EMPTY && !grid[ny][nx].updated_this_frame) {
                    // Fill empty cell with air properties from this cell
                    fill_empty_with_air(nx, ny);
                    
                    // Transfer some properties to the new air cell
                    grid[ny][nx].pressure = grid[y][x].pressure / 2;
                    grid[ny][nx].water = grid[y][x].water / 2;
                    grid[ny][nx].oxygen = grid[y][x].oxygen / 2;
                    grid[ny][nx].mineral = grid[y][x].mineral / 2;
                    
                    // Reduce properties in the source air cell
                    grid[y][x].pressure /= 2;
                    grid[y][x].water /= 2;
                    grid[y][x].oxygen /= 2;
                    grid[y][x].mineral /= 2;
                    
                    grid[y][x].updated_this_frame = true;
                    filled_empty = true;
                    break;
                }
            }
            
            // If we filled an empty cell, skip other movements for this air cell
            if (filled_empty) continue;
            
            // PRIORITY 2: Check for potential water droplet formation
            // Air 2 cells below a border or immovable object may form water droplets
            bool under_immovable = false;
            if (y >= 2) { // Make sure we have room to check above
                if (grid[y-2][x].type == CELL_TYPE_BORDER || 
                    grid[y-2][x].type == CELL_TYPE_ROCK) {
                    under_immovable = true;
                }
            }
            
            // Form water droplets from humid air under surfaces (condensation)
            if (under_immovable && grid[y][x].water > 60 && random_chance() < 5) {
                grid[y][x].type = CELL_TYPE_WATER;
                grid[y][x].baseColor = BLUE;
                grid[y][x].pressure = grid[y][x].nominal_pressure;
                // Preserve some of the water content, adjust other properties
                grid[y][x].oxygen = 5;
                grid[y][x].mineral = 0;
                grid[y][x].updated_this_frame = true;
                continue;
            }
            
            // PRIORITY 3: Move based on density (air with water rises, denser air sinks)
            // Try to move upward if less dense than air above
            if (y > 1 && grid[y-1][x].type == CELL_TYPE_AIR && 
                !grid[y-1][x].updated_this_frame) {
                // Air with more water content is less dense (counterintuitive but fits the comment)
                if (grid[y][x].water > grid[y-1][x].water) {
                    swap_cells(x, y, x, y-1);
                    continue;
                }
                // Compare densities - general case
                else if (calculate_density(&grid[y][x]) < calculate_density(&grid[y-1][x])) {
                    swap_cells(x, y, x, y-1);
                    continue;
                }
            }
            
            // PRIORITY 4: Equalize pressure with adjacent air cells
            bool equalized = false;
            for (int i = 0; i < 8; i++) {
                int nx = x + DIR_X[i];
                int ny = y + DIR_Y[i];
                
                if (nx > 0 && nx < GRID_WIDTH - 1 && ny > 0 && ny < GRID_HEIGHT - 1 && 
                    grid[ny][nx].type == CELL_TYPE_AIR && !grid[ny][nx].updated_this_frame) {
                    // If significant pressure difference, equalize
                    if (abs(grid[y][x].pressure - grid[ny][nx].pressure) > 5) {
                        int avg_pressure = (grid[y][x].pressure + grid[ny][nx].pressure) / 2;
                        grid[y][x].pressure = avg_pressure;
                        grid[ny][nx].pressure = avg_pressure;
                        equalized = true;
                        grid[ny][nx].updated_this_frame = true;
                        break;
                    }
                }
            }
            
            // If we equalized pressure, skip other movements
            if (equalized) continue;
            
            // PRIORITY 5: Random horizontal movement (air diffusion)
            if (random_chance() < 30) { // 30% chance to move horizontally
                int dir = (random_chance() < 50) ? -1 : 1;
                int nx = x + dir;
                
                if (nx > 0 && nx < GRID_WIDTH - 1 && 
                    grid[y][nx].type == CELL_TYPE_AIR && 
                    !grid[y][nx].updated_this_frame) {
                    // Small chance to swap even without pressure differential
                    swap_cells(x, y, nx, y);
                    continue;
                }
            }
            
            // PRIORITY 6: Check for high pressure and try to move accordingly
            if (grid[y][x].pressure > grid[y][x].nominal_pressure * 1.5f) {
                // High pressure air tries to move to lower pressure areas
                int lowest_pressure_dir = -1;
                int lowest_pressure = grid[y][x].pressure;
                
                // Find direction with lowest pressure
                for (int i = 0; i < 8; i++) {
                    int nx = x + DIR_X[i];
                    int ny = y + DIR_Y[i];
                    
                    if (nx > 0 && nx < GRID_WIDTH - 1 && ny > 0 && ny < GRID_HEIGHT - 1 && 
                        grid[ny][nx].type == CELL_TYPE_AIR && !grid[ny][nx].updated_this_frame) {
                        if (grid[ny][nx].pressure < lowest_pressure) {
                            lowest_pressure = grid[ny][nx].pressure;
                            lowest_pressure_dir = i;
                        }
                    }
                }
                
                // Move to the lowest pressure area if found
                if (lowest_pressure_dir >= 0) {
                    int nx = x + DIR_X[lowest_pressure_dir];
                    int ny = y + DIR_Y[lowest_pressure_dir];
                    swap_cells(x, y, nx, ny);
                }
            }
        }
    }
}

// Water movement - water falls and flows horizontally
/*
do not remove this comment, it is important to the function of the code.
uses push_cellvoid to displace things
check the 3 cells below and diagonally below the cell, and if they are empty, or air, we will move portions of the water into them, then this cell becomes empty.
check the 3 cells below and diagonally below the cell if there is soil, rock or border in the cell below me, i am supported.
check the 3 cells below and diagonally below the cell, if they are water, and below nominal pressure, we will move portions of the water into them, then this cell becomes air with the remaining water in it, and nominal air pressure.
check the 3 cells below and diagonally below the cell if those are all at or above nominal pressure, or supported, we consider ourself supported, and we will not fall, or try to transfer water to the cells below us.
if we are above nominal pressure, we already tried moving our extra pressure down, so now we check if we can move sideways. check both directions, and either divide the pressure betwenen them or move half your water/pressure into the empty/air one.
if we are still above nominal pressure we will check the 3 cells above and diagonally above the cell, and try to divide our excess pressure into them.
*/
void update_water(void) {
    // Process from top to bottom for proper pressure propagation
    for (int y = 1; y < GRID_HEIGHT - 1; y++) {
        for (int x = 1; x < GRID_WIDTH - 1; x++) {
            if (grid[y][x].updated_this_frame || grid[y][x].type != CELL_TYPE_WATER)
                continue;
                
            // First, check if we're supported from below (solid objects)
            bool is_supported = false;
            if (grid[y+1][x].type == CELL_TYPE_SOIL || 
                grid[y+1][x].type == CELL_TYPE_ROCK || 
                grid[y+1][x].type == CELL_TYPE_BORDER || 
                grid[y+1][x].type == CELL_TYPE_PLANT) {
                is_supported = true;
            }
            
            // Check if water below us is at or above nominal pressure (also counts as support)
            bool water_pressure_below_high = false;
            if (grid[y+1][x].type == CELL_TYPE_WATER && 
                grid[y+1][x].pressure >= grid[y+1][x].nominal_pressure) {
                water_pressure_below_high = true;
                is_supported = true;
            }
            
            // Check diagonal cells below for support
            for (int dx = -1; dx <= 1; dx += 2) { // Check diagonals (-1 and +1)
                int nx = x + dx;
                if (nx <= 0 || nx >= GRID_WIDTH - 1) continue;
                
                if (grid[y+1][nx].type == CELL_TYPE_SOIL || 
                    grid[y+1][nx].type == CELL_TYPE_ROCK || 
                    grid[y+1][nx].type == CELL_TYPE_BORDER || 
                    grid[y+1][nx].type == CELL_TYPE_PLANT) {
                    // Diagonal support detected
                    if (random_chance() < 50) { // 50% chance to consider diagonal support
                        is_supported = true;
                    }
                }
                
                // Check if diagonal water is at or above nominal pressure
                if (grid[y+1][nx].type == CELL_TYPE_WATER && 
                    grid[y+1][nx].pressure >= grid[y+1][nx].nominal_pressure) {
                    water_pressure_below_high = true;
                    if (random_chance() < 50) { // 50% chance to consider diagonal water pressure
                        is_supported = true;
                    }
                }
            }
            
            // If not supported, try to move down or displace air below
            if (!is_supported) {
                // Check directly below first
                if (grid[y+1][x].type == CELL_TYPE_AIR || grid[y+1][x].type == CELL_TYPE_EMPTY) {
                    if (!grid[y+1][x].updated_this_frame) {
                        if (grid[y+1][x].type == CELL_TYPE_AIR) {
                            push_cellvoid(x, y, x, y+1);
                        } else { // EMPTY
                            swap_cells(x, y, x, y+1);
                        }
                        grid[y+1][x].is_falling = true;
                        continue;
                    }
                }
                
                // Try diagonally down
                int dir = (random_chance() < 50) ? -1 : 1; // Randomly choose left or right
                if (x+dir > 0 && x+dir < GRID_WIDTH-1 && 
                    (grid[y+1][x+dir].type == CELL_TYPE_AIR || grid[y+1][x+dir].type == CELL_TYPE_EMPTY) && 
                    !grid[y+1][x+dir].updated_this_frame) {
                    if (grid[y+1][x+dir].type == CELL_TYPE_AIR) {
                        push_cellvoid(x, y, x+dir, y+1);
                    } else { // EMPTY
                        swap_cells(x, y, x+dir, y+1);
                    }
                    grid[y+1][x+dir].is_falling = true;
                    continue;
                }
                
                // Try the other diagonal if first one failed
                dir = -dir;
                if (x+dir > 0 && x+dir < GRID_WIDTH-1 && 
                    (grid[y+1][x+dir].type == CELL_TYPE_AIR || grid[y+1][x+dir].type == CELL_TYPE_EMPTY) && 
                    !grid[y+1][x+dir].updated_this_frame) {
                    if (grid[y+1][x+dir].type == CELL_TYPE_AIR) {
                        push_cellvoid(x, y, x+dir, y+1);
                    } else { // EMPTY
                        swap_cells(x, y, x+dir, y+1);
                    }
                    grid[y+1][x+dir].is_falling = true;
                    continue;
                }
                
                // If can't move down, try to flow horizontally
                dir = (random_chance() < 50) ? -1 : 1; // Randomly choose left or right
                if (x+dir > 0 && x+dir < GRID_WIDTH-1 && 
                    (grid[y][x+dir].type == CELL_TYPE_AIR || grid[y][x+dir].type == CELL_TYPE_EMPTY) && 
                    !grid[y][x+dir].updated_this_frame) {
                    if (grid[y][x+dir].type == CELL_TYPE_AIR) {
                        push_cellvoid(x, y, x+dir, y);
                    } else { // EMPTY
                        swap_cells(x, y, x+dir, y);
                    }
                    continue;
                }
            }
            
            // If we are supported (or couldn't move down), build pressure
            if (is_supported) {
                // Water that can't move down builds pressure
                grid[y][x].pressure += 1;
                
                // Transfer some pressure to water below if it exists and below nominal pressure
                if (grid[y+1][x].type == CELL_TYPE_WATER && 
                    grid[y+1][x].pressure < grid[y+1][x].nominal_pressure) {
                    // Transfer pressure downward
                    int transfer = (grid[y][x].pressure - grid[y][x].nominal_pressure) / 2;
                    if (transfer > 0) {
                        grid[y+1][x].pressure += transfer;
                        grid[y][x].pressure -= transfer;
                    }
                }
                
                // If above nominal pressure, try to move excess pressure sideways
                if (grid[y][x].pressure > grid[y][x].nominal_pressure * 1.2f) {
                    // Check both left and right
                    bool moved_sideways = false;
                    for (int dx = -1; dx <= 1; dx += 2) { // Check left and right (-1 and +1)
                        int nx = x + dx;
                        if (nx <= 0 || nx >= GRID_WIDTH-1 || grid[y][nx].updated_this_frame) continue;
                        
                        // If air/empty cell, move there with push_cellvoid
                        if (grid[y][nx].type == CELL_TYPE_AIR || grid[y][nx].type == CELL_TYPE_EMPTY) {
                            if (grid[y][nx].type == CELL_TYPE_AIR) {
                                push_cellvoid(x, y, nx, y);
                            } else { // EMPTY
                                swap_cells(x, y, nx, y);
                            }
                            moved_sideways = true;
                            break;
                        }
                        
                        // If water cell with lower pressure, transfer pressure
                        if (grid[y][nx].type == CELL_TYPE_WATER && 
                            grid[y][nx].pressure < grid[y][nx].nominal_pressure) {
                            int transfer = (grid[y][x].pressure - grid[y][x].nominal_pressure) / 2;
                            if (transfer > 0) {
                                grid[y][nx].pressure += transfer;
                                grid[y][x].pressure -= transfer;
                                moved_sideways = true;
                                break;
                            }
                        }
                    }
                    
                    // If still above nominal pressure and couldn't move sideways, try to move upward
                    if (!moved_sideways && grid[y][x].pressure > grid[y][x].nominal_pressure * 1.5f) {
                        // Check directly above
                        if (y > 1 && !grid[y-1][x].updated_this_frame) {
                            if (grid[y-1][x].type == CELL_TYPE_AIR || grid[y-1][x].type == CELL_TYPE_EMPTY) {
                                if (grid[y-1][x].type == CELL_TYPE_AIR) {
                                    push_cellvoid(x, y, x, y-1);
                                } else { // EMPTY
                                    swap_cells(x, y, x, y-1);
                                }
                                continue;
                            }
                            
                            // If water above with lower pressure, transfer pressure
                            if (grid[y-1][x].type == CELL_TYPE_WATER && 
                                grid[y-1][x].pressure < grid[y-1][x].nominal_pressure) {
                                int transfer = (grid[y][x].pressure - grid[y][x].nominal_pressure) / 2;
                                if (transfer > 0) {
                                    grid[y-1][x].pressure += transfer;
                                    grid[y][x].pressure -= transfer;
                                    continue;
                                }
                            }
                        }
                        
                        // Try diagonally above
                        int dir = (random_chance() < 50) ? -1 : 1; // Randomly choose left or right
                        if (y > 1 && x+dir > 0 && x+dir < GRID_WIDTH-1 && 
                            !grid[y-1][x+dir].updated_this_frame) {
                            if (grid[y-1][x+dir].type == CELL_TYPE_AIR || grid[y-1][x+dir].type == CELL_TYPE_EMPTY) {
                                if (grid[y-1][x+dir].type == CELL_TYPE_AIR) {
                                    push_cellvoid(x, y, x+dir, y-1);
                                } else { // EMPTY
                                    swap_cells(x, y, x+dir, y-1);
                                }
                                continue;
                            }
                            
                            // If water above with lower pressure, transfer pressure
                            if (grid[y-1][x+dir].type == CELL_TYPE_WATER && 
                                grid[y-1][x+dir].pressure < grid[y-1][x+dir].nominal_pressure) {
                                int transfer = (grid[y][x].pressure - grid[y][x].nominal_pressure) / 2;
                                if (transfer > 0) {
                                    grid[y-1][x+dir].pressure += transfer;
                                    grid[y][x].pressure -= transfer;
                                    continue;
                                }
                            }
                        }
                    }
                }
            } else {
                // Water that can move gradually returns to nominal pressure
                if (grid[y][x].pressure > grid[y][x].nominal_pressure) {
                    grid[y][x].pressure = (grid[y][x].pressure * 9 + grid[y][x].nominal_pressure) / 10;
                }
            }
        }
    }
}

// Soil/Sand movement - falls and piles up
/*
as sand falls it uses push_cellvoid to displace air and water. 
*/
void update_soil(void) {
    // Process from bottom to top for falling
    for (int y = GRID_HEIGHT - 2; y > 0; y--) {
        for (int x = 1; x < GRID_WIDTH - 1; x++) {
            if (grid[y][x].updated_this_frame || grid[y][x].type != CELL_TYPE_SOIL)
                continue;
                
            // Try to move down first
            if (grid[y+1][x].type == CELL_TYPE_AIR || grid[y+1][x].type == CELL_TYPE_WATER) {
                if (!grid[y+1][x].updated_this_frame) {
                    swap_cells(x, y, x, y+1);
                    grid[y+1][x].is_falling = true;
                }
            }
            // If can't move directly down, try diagonally
            else if (!grid[y][x].is_falling) {
                int dir = (random_chance() < 50) ? -1 : 1; // Randomly choose left or right
                if (x+dir > 0 && x+dir < GRID_WIDTH - 1 && 
                    (grid[y+1][x+dir].type == CELL_TYPE_AIR || grid[y+1][x+dir].type == CELL_TYPE_WATER) && 
                    !grid[y+1][x+dir].updated_this_frame) {
                    swap_cells(x, y, x+dir, y+1);
                    grid[y+1][x+dir].is_falling = true;
                }
            }
        }
    }
}

// Moss update - similar to soil but with growth
void update_moss(void) {
    // First handle movement like soil
    for (int y = GRID_HEIGHT - 2; y > 0; y--) {
        for (int x = 1; x < GRID_WIDTH - 1; x++) {
            if (grid[y][x].updated_this_frame || grid[y][x].type != CELL_TYPE_MOSS)
                continue;
                
            // Try to move down first
            if (grid[y+1][x].type == CELL_TYPE_AIR || grid[y+1][x].type == CELL_TYPE_WATER) {
                if (!grid[y+1][x].updated_this_frame) {
                    swap_cells(x, y, x, y+1);
                    grid[y+1][x].is_falling = true;
                    continue;
                }
            }
            // If can't move directly down, try diagonally
            else if (!grid[y][x].is_falling && random_chance() < 50) { // 50% chance
                int dir = (random_chance() < 50) ? -1 : 1; // Randomly choose left or right
                if (x+dir > 0 && x+dir < GRID_WIDTH - 1 && 
                    (grid[y+1][x+dir].type == CELL_TYPE_AIR || grid[y+1][x+dir].type == CELL_TYPE_WATER) && 
                    !grid[y+1][x+dir].updated_this_frame) {
                    swap_cells(x, y, x+dir, y+1);
                    grid[y+1][x+dir].is_falling = true;
                    continue;
                }
            }
            
            // Growth: if moss has enough water and oxygen, it can spread to adjacent soil
            if (!grid[y][x].is_falling && grid[y][x].water > 40 && grid[y][x].oxygen > 10) {
                // Try to spread to adjacent soil
                int dirIndex = rand() % 8; // Randomly choose one of 8 directions
                int nx = x + DIR_X[dirIndex];
                int ny = y + DIR_Y[dirIndex];
                
                if (nx > 0 && nx < GRID_WIDTH - 1 && ny > 0 && ny < GRID_HEIGHT - 1 && 
                    grid[ny][nx].type == CELL_TYPE_SOIL && 
                    grid[ny][nx].water > 30 && 
                    random_chance() < 5) { // 5% chance to spread
                    grid[ny][nx].type = CELL_TYPE_MOSS;
                    grid[ny][nx].baseColor = (Color){20, 100 + (rand() % 40), 20, 255}; // Slightly varied green
                    grid[ny][nx].oxygen = grid[y][x].oxygen - 5;
                    grid[ny][nx].updated_this_frame = true;
                    grid[y][x].oxygen -= 5; // Reduce oxygen of the parent moss
                }
            }
        }
    }
}

// Plant update function
void update_plant(void) {
    for (int y = GRID_HEIGHT - 2; y > 0; y--) {
        for (int x = 1; x < GRID_WIDTH - 1; x++) {
            if (grid[y][x].updated_this_frame || grid[y][x].type != CELL_TYPE_PLANT)
                continue;
                
            // Increment age
            grid[y][x].age++;
            
            // Check if plant has water and oxygen to grow
            if (grid[y][x].water > 20 && grid[y][x].oxygen > 5) {
                // Try to grow upward if there's air above
                if (y > 1 && grid[y-1][x].type == CELL_TYPE_AIR && random_chance() < 3) {
                    grid[y-1][x].type = CELL_TYPE_PLANT;
                    grid[y-1][x].baseColor = (Color){20, 150 + (rand() % 50), 20, 255}; // Green
                    grid[y-1][x].oxygen = grid[y][x].oxygen - 5;
                    grid[y-1][x].water = grid[y][x].water / 2;
                    grid[y-1][x].updated_this_frame = true;
                    grid[y][x].oxygen -= 5; // Use oxygen for growth
                }
            }
            
            // Absorb water from adjacent water cells
            for (int i = 0; i < 8; i++) {
                int nx = x + DIR_X[i];
                int ny = y + DIR_Y[i];
                
                if (nx > 0 && nx < GRID_WIDTH - 1 && ny > 0 && ny < GRID_HEIGHT - 1 && 
                    grid[ny][nx].type == CELL_TYPE_WATER && 
                    grid[y][x].water < 90) {
                    grid[y][x].water += 5;
                    grid[ny][nx].water -= 5; // Remove water from source
                    if (grid[y][x].water > 100) grid[y][x].water = 100;
                    break;
                }
            }
        }
    }
}

// Erosion and sedimentation
void update_erosion_sedimentation(void) {
    for (int y = 1; y < GRID_HEIGHT - 1; y++) {
        for (int x = 1; x < GRID_WIDTH - 1; x++) {
            if (grid[y][x].updated_this_frame || grid[y][x].type != CELL_TYPE_WATER)
                continue;
                
            // Count water cells above to calculate pressure
            int water_count = 0;
            for (int dy = 0; dy <= 2; dy++) {
                for (int dx = -2; dx <= 2; dx++) {
                    int nx = x + dx;
                    int ny = y - dy;
                    if (nx > 0 && nx < GRID_WIDTH - 1 && ny > 0 && ny < GRID_HEIGHT - 1 && 
                        grid[ny][nx].type == CELL_TYPE_WATER) {
                        water_count++;
                    }
                }
            }
            
            // Erosion: if enough water pressure and soil below, erode soil
            if (water_count >= 4 && y < GRID_HEIGHT - 1 && grid[y+1][x].type == CELL_TYPE_SOIL && 
                random_chance() < 10) { // 10% chance
                // Erode the soil
                grid[y+1][x].type = CELL_TYPE_WATER;
                grid[y+1][x].baseColor = BLUE;
                grid[y+1][x].water = 90;
                grid[y+1][x].oxygen = 5;
                grid[y+1][x].mineral = 5;
                grid[y+1][x].updated_this_frame = true;
            }
            
            // Sedimentation: if water has been still and has soil above it
            if (!grid[y][x].is_falling && y > 1 && grid[y-1][x].type == CELL_TYPE_SOIL && 
                random_chance() < 5) { // 5% chance
                // Create sediment (soil) below
                if (y < GRID_HEIGHT - 1 && grid[y+1][x].type == CELL_TYPE_AIR) {
                    grid[y+1][x].type = CELL_TYPE_SOIL;
                    grid[y+1][x].baseColor = BROWN;
                    grid[y+1][x].water = grid[y][x].water / 2;
                    grid[y+1][x].oxygen = 10;
                    grid[y+1][x].mineral = 60;
                    grid[y+1][x].updated_this_frame = true;
                }
            }
        }
    }
}

// Evaporation and precipitation
void update_evaporation_precipitation(void) {
    for (int y = 1; y < GRID_HEIGHT - 1; y++) {
        for (int x = 1; x < GRID_WIDTH - 1; x++) {
            if (grid[y][x].updated_this_frame)
                continue;
                
            // Water evaporation: water transfers to air above
            if (grid[y][x].type == CELL_TYPE_WATER && y > 1 && 
                grid[y-1][x].type == CELL_TYPE_AIR && 
                grid[y-1][x].water < 50 && 
                random_chance() < 2) { // 2% chance per update
                grid[y-1][x].water += 10;
                if (grid[y-1][x].water > 100) grid[y-1][x].water = 100;
                grid[y][x].water -= 5;
                
                // If water loses too much water content, it might disappear
                if (grid[y][x].water < 20 && random_chance() < 10) {
                    grid[y][x].type = CELL_TYPE_AIR;
                    grid[y][x].baseColor = SKYBLUE;
                    grid[y][x].water = 20;
                    grid[y][x].oxygen = 80;
                    grid[y][x].mineral = 0;
                    grid[y][x].updated_this_frame = true;
                }
            }
            
            // Precipitation: humid air forms water
            if (grid[y][x].type == CELL_TYPE_AIR && grid[y][x].water > 70 && 
                grid[y][x].temperature <= grid[y][x].dewpoint &&
                random_chance() < 5) { // 5% chance
                grid[y][x].type = CELL_TYPE_WATER;
                grid[y][x].baseColor = BLUE;
                grid[y][x].water = 90;
                grid[y][x].oxygen = 5;
                grid[y][x].mineral = 5;
                grid[y][x].updated_this_frame = true;
            }
        }
    }
}

// Fill an empty cell with air at appropriate pressure
void fill_empty_with_air(int x, int y) {
    // Count adjacent air cells and sum their pressures
    int air_count = 0;
    float total_pressure = 0;
    
    // Check the surrounding cells (excluding cells above which would be border)
    for (int dy = 0; dy <= 1; dy++) { // Check current row and the row below
        for (int dx = -1; dx <= 1; dx++) {
            int nx = x + dx;
            int ny = y + dy;
            
            // Skip out of bounds cells
            if (nx <= 0 || nx >= GRID_WIDTH - 1 || ny <= 0 || ny >= GRID_HEIGHT - 1) {
                continue;
            }
            
            // If it's an air cell, add its pressure to the total
            if (grid[ny][nx].type == CELL_TYPE_AIR) {
                air_count++;
                total_pressure += grid[ny][nx].pressure;
            }
        }
    }
    
    // Initialize the empty cell as air
    InitializeCellDefaults(&grid[y][x], CELL_TYPE_AIR);
    
    // Set pressure based on surrounding air cells or use nominal pressure if no air cells found
    if (air_count > 0) {
        grid[y][x].pressure = total_pressure / air_count; // Average pressure
    } else {
        grid[y][x].pressure = grid[y][x].nominal_pressure; // Use nominal pressure for air
    }
    
    // Mark as updated this frame
    grid[y][x].updated_this_frame = true;
}

// Process empty cells below the top border row and replace them with appropriate air cells
void process_cells_below_border(void) {
    int y = 1; // The first row below the border (border is at y=0)
    
    for (int x = 1; x < GRID_WIDTH - 1; x++) {
        // Check if there's an empty cell directly below the border
        if (grid[y][x].type == CELL_TYPE_EMPTY) {
            // Fill the empty cell with appropriate air
            fill_empty_with_air(x, y);
        }
    }
}

// Main update function
void updateCells(void) {
    // Initialize random seed if not done already
    static bool initialized = false;
    if (!initialized) {
        srand(time(NULL));
        initialized = true;
    }
    
    // Reset updated_this_frame flag for all cells
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x].updated_this_frame = false;
        }
    }
    
    // Update cells in order of priority
    process_cells_below_border();        // First process cells below the border
    update_evaporation_precipitation();  // Then handle evaporation and precipitation
    update_erosion_sedimentation();      // Then erosion and sedimentation
    update_water();                      // Water movement
    update_soil();                       // Soil movement
    update_moss();                       // Moss movement and growth
    update_plant();                      // Plant growth
    update_air();                        // Air movement (last since it's lowest priority)
}

