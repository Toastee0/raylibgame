#include "raylib.h"
#include "updatecells.h"
#include "grid.h"
#include "cell_types.h"
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

// Function to calculate cell density based on type and moisture
float calculate_density(GridCell* cell) {
    float density = BASE_DENSITIES[cell->type];
    
    // Moisture increases density slightly
    density += (cell->moisture / 100.0f) * 2.0f;
    
    return density;
}

// Function to swap two cells
void swap_cells(int x1, int y1, int x2, int y2) {
    GridCell temp = grid[y1][x1];
    
    // Keep the position values correct
    Vector2 pos1 = temp.position;
    
    grid[y1][x1] = grid[y2][x2];
    grid[y1][x1].position = pos1;
    
    grid[y2][x2] = temp;
    grid[y2][x2].position = grid[y2][x2].position;
    
    // Mark both cells as updated
    grid[y1][x1].updated_this_frame = true;
    grid[y2][x2].updated_this_frame = true;
}

// Air movement - air rises or moves horizontally
void update_air(void) {
    // Process from top to bottom to allow air to rise
    for (int y = 1; y < GRID_HEIGHT - 1; y++) {
        for (int x = 1; x < GRID_WIDTH - 1; x++) {
            // Skip if already processed this frame
            if (grid[y][x].updated_this_frame || grid[y][x].type != CELL_TYPE_AIR)
                continue;
                
            // Air tries to move up
            if (y > 1 && grid[y-1][x].type == CELL_TYPE_AIR && 
                !grid[y-1][x].updated_this_frame) {
                // Compare densities - denser air sinks, less dense rises
                if (calculate_density(&grid[y][x]) < calculate_density(&grid[y-1][x])) {
                    swap_cells(x, y, x, y-1);
                }
            }
            // Try moving left or right randomly if can't move up
            else {
                int dir = (random_chance() < 50) ? -1 : 1;
                int nx = x + dir;
                
                if (nx > 0 && nx < GRID_WIDTH - 1 && 
                    grid[y][nx].type == CELL_TYPE_AIR && 
                    !grid[y][nx].updated_this_frame) {
                    if (random_chance() < 25) { // 25% chance to move horizontally
                        swap_cells(x, y, nx, y);
                    }
                }
            }
        }
    }
}

// Water movement - water falls and flows horizontally
void update_water(void) {
    // Process from bottom to top for falling
    for (int y = GRID_HEIGHT - 2; y > 0; y--) {
        for (int x = 1; x < GRID_WIDTH - 1; x++) {
            if (grid[y][x].updated_this_frame || grid[y][x].type != CELL_TYPE_WATER)
                continue;
                
            // Try to move down first
            if (grid[y+1][x].type == CELL_TYPE_AIR && !grid[y+1][x].updated_this_frame) {
                swap_cells(x, y, x, y+1);
                grid[y+1][x].is_falling = true;
            }
            // If can't move down, try diagonally down
            else if (!grid[y][x].is_falling && random_chance() < 80) { // 80% chance when not already falling
                int dir = (random_chance() < 50) ? -1 : 1; // Randomly choose left or right
                if (x+dir > 0 && x+dir < GRID_WIDTH - 1 && 
                    grid[y+1][x+dir].type == CELL_TYPE_AIR && 
                    !grid[y+1][x+dir].updated_this_frame) {
                    swap_cells(x, y, x+dir, y+1);
                    grid[y+1][x+dir].is_falling = true;
                }
            }
            // If can't move down or diagonally, try to flow horizontally
            else if (!grid[y][x].is_falling && random_chance() < 50) { // 50% chance when not already falling
                int dir = (random_chance() < 50) ? -1 : 1; // Randomly choose left or right
                if (x+dir > 0 && x+dir < GRID_WIDTH - 1 && 
                    grid[y][x+dir].type == CELL_TYPE_AIR && 
                    !grid[y][x+dir].updated_this_frame) {
                    swap_cells(x, y, x+dir, y);
                }
            }
        }
    }
}

// Soil/Sand movement - falls and piles up
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
            
            // Growth: if moss has enough moisture and energy, it can spread to adjacent soil
            if (!grid[y][x].is_falling && grid[y][x].moisture > 40 && grid[y][x].Energy > 3) {
                // Try to spread to adjacent soil
                int dirIndex = rand() % 8; // Randomly choose one of 8 directions
                int nx = x + DIR_X[dirIndex];
                int ny = y + DIR_Y[dirIndex];
                
                if (nx > 0 && nx < GRID_WIDTH - 1 && ny > 0 && ny < GRID_HEIGHT - 1 && 
                    grid[ny][nx].type == CELL_TYPE_SOIL && 
                    grid[ny][nx].moisture > 30 && 
                    random_chance() < 5) { // 5% chance to spread
                    grid[ny][nx].type = CELL_TYPE_MOSS;
                    grid[ny][nx].baseColor = (Color){20, 100 + (rand() % 40), 20, 255}; // Slightly varied green
                    grid[ny][nx].Energy = 3;
                    grid[ny][nx].updated_this_frame = true;
                    grid[y][x].Energy--; // Reduce energy of the parent moss
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
            
            // Check if plant has moisture and energy to grow
            if (grid[y][x].moisture > 20 && grid[y][x].Energy > 2) {
                // Try to grow upward if there's air above
                if (y > 1 && grid[y-1][x].type == CELL_TYPE_AIR && random_chance() < 3) {
                    grid[y-1][x].type = CELL_TYPE_PLANT;
                    grid[y-1][x].baseColor = (Color){20, 150 + (rand() % 50), 20, 255}; // Green
                    grid[y-1][x].Energy = grid[y][x].Energy - 1;
                    grid[y-1][x].moisture = grid[y][x].moisture / 2;
                    grid[y-1][x].updated_this_frame = true;
                    grid[y][x].Energy--;
                }
            }
            
            // Absorb moisture from adjacent water
            for (int i = 0; i < 8; i++) {
                int nx = x + DIR_X[i];
                int ny = y + DIR_Y[i];
                
                if (nx > 0 && nx < GRID_WIDTH - 1 && ny > 0 && ny < GRID_HEIGHT - 1 && 
                    grid[ny][nx].type == CELL_TYPE_WATER && 
                    grid[y][x].moisture < 90) {
                    grid[y][x].moisture += 5;
                    if (grid[y][x].moisture > 100) grid[y][x].moisture = 100;
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
                grid[y+1][x].moisture = 100;
                grid[y+1][x].updated_this_frame = true;
            }
            
            // Sedimentation: if water has been still and has soil above it
            if (!grid[y][x].is_falling && y > 1 && grid[y-1][x].type == CELL_TYPE_SOIL && 
                random_chance() < 5) { // 5% chance
                // Create sediment (soil) below
                if (y < GRID_HEIGHT - 1 && grid[y+1][x].type == CELL_TYPE_AIR) {
                    grid[y+1][x].type = CELL_TYPE_SOIL;
                    grid[y+1][x].baseColor = BROWN;
                    grid[y+1][x].moisture = grid[y][x].moisture / 2;
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
                
            // Water evaporation: water transfers moisture to air above
            if (grid[y][x].type == CELL_TYPE_WATER && y > 1 && 
                grid[y-1][x].type == CELL_TYPE_AIR && 
                grid[y-1][x].moisture < 95 && 
                random_chance() < 2) { // 2% chance per update
                grid[y-1][x].moisture += 10;
                if (grid[y-1][x].moisture > 100) grid[y-1][x].moisture = 100;
                grid[y][x].moisture -= 5;
                
                // If water loses too much moisture, it might disappear
                if (grid[y][x].moisture < 20 && random_chance() < 10) {
                    grid[y][x].type = CELL_TYPE_AIR;
                    grid[y][x].baseColor = SKYBLUE;
                    grid[y][x].updated_this_frame = true;
                }
            }
            
            // Precipitation: moist air forms water
            if (grid[y][x].type == CELL_TYPE_AIR && grid[y][x].moisture > 95 && 
                random_chance() < 5) { // 5% chance
                grid[y][x].type = CELL_TYPE_WATER;
                grid[y][x].baseColor = BLUE;
                grid[y][x].moisture = 100;
                grid[y][x].updated_this_frame = true;
            }
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
    update_evaporation_precipitation();  // First handle evaporation and precipitation
    update_erosion_sedimentation();      // Then erosion and sedimentation
    update_water();                      // Water movement
    update_soil();                       // Soil movement
    update_moss();                       // Moss movement and growth
    update_plant();                      // Plant growth
    update_air();                        // Air movement (last since it's lowest priority)
}

