#include "cell_actions.h"
#include "grid.h"
#include "cell_types.h"
#include "cell_defaults.h"  // Add this include
#include <stdio.h>
#include <stdbool.h>
#include <math.h>  // Add this include for sqrtf

// Move cell contents from one position to another

// Swap the contents of two cells


// Place soil at the given position
void PlaceSoil(Vector2 position) {
    int x = (int)position.x;
    int y = (int)position.y;
    
    // Ensure position is within grid bounds
    if(x < 0 || x > GRID_WIDTH - 1 || y < 0 || y > GRID_HEIGHT - 1) return;
    
    // Initialize with defaults first
    InitializeCellDefaults(&grid[y][x], CELL_TYPE_SOIL);
}

// Place water at the given position
void PlaceWater(Vector2 position) {
    int x = (int)position.x;
    int y = (int)position.y;
    
    // Ensure position is within grid bounds
    if(x < 0 || x > GRID_WIDTH - 1 || y < 0 || y > GRID_HEIGHT - 1) return;
    
    // Initialize with defaults first
    InitializeCellDefaults(&grid[y][x], CELL_TYPE_WATER);
    
    // Set clean water properties
    grid[y][x].water = 100;      // Pure water content
    grid[y][x].pressure = grid[y][x].nominal_pressure;  // Use nominal pressure
    grid[y][x].oxygen = 5;       // Small amount of dissolved oxygen
    grid[y][x].mineral = 0;      // Clean water with no minerals
    
    // Vibrant blue color for clean water
    grid[y][x].baseColor = (Color){
        0,          // No red (pure blue)
        120,        // Medium green component for cyan-blue
        255,        // Full blue
        255         // Full opacity
    };
    
    // Set falling state to false initially
    grid[y][x].is_falling = false;
    
    // Mark as updated this frame to prevent immediate processing
    grid[y][x].updated_this_frame = true;
}

// Place rock at the given position
void PlaceRock(Vector2 position) {
    int x = (int)position.x;
    int y = (int)position.y;
    
    // Ensure position is within grid bounds
    if(x < 0 || x > GRID_WIDTH - 1 || y < 0 || y > GRID_HEIGHT - 1) return;
    
    // Initialize with defaults first
    InitializeCellDefaults(&grid[y][x], CELL_TYPE_ROCK);
    
    // Rocks can have slight color variation
    int variation = GetRandomValue(-15, 15);
    grid[y][x].baseColor = (Color){
        128 + variation,  // Base gray with variation
        128 + variation,
        128 + variation,
        255
    };
}

// Place plant at the given position
void PlacePlant(Vector2 position) {
    int x = (int)position.x;
    int y = (int)position.y;
    
    // Ensure position is within grid bounds
    if(x < 0 || x > GRID_WIDTH - 1 || y < 0 || y > GRID_HEIGHT - 1) return;
    
    // Only allow plants to grow on soil
    if(grid[y][x].type != CELL_TYPE_SOIL && grid[y][x].type != CELL_TYPE_AIR) {
        return;
    }
    
    // Initialize with defaults first
    InitializeCellDefaults(&grid[y][x], CELL_TYPE_PLANT);
    
    // Add some color variation to plants
    int greenVariation = GetRandomValue(-20, 20);
    grid[y][x].baseColor = (Color){
        20 + GetRandomValue(0, 30),         // Small amount of red
        150 + greenVariation,               // Varied green
        40 + GetRandomValue(-20, 20),       // Small amount of blue
        255
    };
    
      // Initialize age (starts at 0)
    grid[y][x].age = 0;
    
    // Plants start with moderate water content
    grid[y][x].water = 50 + GetRandomValue(-10, 10);
    
    // Set a unique ID if tracking plants individually
    static int nextPlantID = 1;
    grid[y][x].objectID = nextPlantID++;
}

// Place moss at the given position
void PlaceMoss(Vector2 position) {
    int x = (int)position.x;
    int y = (int)position.y;
    
    // Ensure position is within grid bounds
    if(x < 0 || x > GRID_WIDTH - 1 || y < 0 || y > GRID_HEIGHT - 1) return;
    
    // Initialize with defaults first
    InitializeCellDefaults(&grid[y][x], CELL_TYPE_MOSS);
    
    // Moss has a darker green shade with some variation
    int greenVariation = GetRandomValue(-10, 10);
    grid[y][x].baseColor = (Color){
        10 + GetRandomValue(0, 10),        // Almost no red
        80 + greenVariation,               // Dark green with variation
        30 + GetRandomValue(-10, 10),      // Small amount of blue
        255
    };
    
   
    
    // Initialize age (starts at 0)
    grid[y][x].age = 0;
    
    // Moss prefers higher water content
    grid[y][x].water = 70 + GetRandomValue(-5, 15);
}

// Place air at the given position 
void PlaceAir(Vector2 position) {
    int x = (int)position.x;
    int y = (int)position.y;
    
    // Ensure position is within grid bounds
    if(x < 0 || x > GRID_WIDTH - 1 || y < 0 || y > GRID_HEIGHT - 1) return;
    
    // Initialize with defaults first
    InitializeCellDefaults(&grid[y][x], CELL_TYPE_AIR);
    
    // Air can have slight water content variation
    grid[y][x].water = GetRandomValue(5, 15);
    
    // Update color based on water content (invisible until high water content)
    if (grid[y][x].water > 75) {
        int brightness = (grid[y][x].water - 75) * (255 / 25);
        grid[y][x].baseColor = (Color){brightness, brightness, brightness, 255};
    } else {
        grid[y][x].baseColor = BLACK;  // Invisible air
    }
}

// Place cells in a circular pattern centered at the given position
void PlaceCircularPattern(int centerX, int centerY, int cellType, int radius) {
    // Special case for brush size 1: just place a single cell at the center
    if (radius == 1) {
        Vector2 position = {(float)centerX, (float)centerY};
        switch (cellType) {
            case CELL_TYPE_AIR:
                PlaceAir(position);
                break;
            case CELL_TYPE_SOIL:
                PlaceSoil(position);
                break;
            case CELL_TYPE_WATER:
                PlaceWater(position);
                break;
            case CELL_TYPE_PLANT:
                PlacePlant(position);
                break;
            case CELL_TYPE_ROCK:
                PlaceRock(position);
                break;
            case CELL_TYPE_MOSS:
                PlaceMoss(position);
                break;
        }
        return;
    }
    
    // For larger brushes, use radius-1 to get the right size
    float actualRadius = radius - 1.0f;
    
    // Place cells in a circular pattern
    for (int y = centerY - radius; y <= centerY + radius; y++) {
        for (int x = centerX - radius; x <= centerX + radius; x++) {
            // Skip out of bounds cells
            if (x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) {
                continue;
            }
            
            // Calculate distance from center
            float distance = sqrtf((float)((x - centerX) * (x - centerX) + (y - centerY) * (y - centerY)));
            
            // Place cell if within radius
            if (distance <= actualRadius) {
                Vector2 position = {(float)x, (float)y};
                switch (cellType) {
                    case CELL_TYPE_AIR:
                        PlaceAir(position);
                        break;
                    case CELL_TYPE_SOIL:
                        PlaceSoil(position);
                        break;
                    case CELL_TYPE_WATER:
                        PlaceWater(position);
                        break;
                    case CELL_TYPE_PLANT:
                        PlacePlant(position);
                        break;
                    case CELL_TYPE_ROCK:
                        PlaceRock(position);
                        break;
                    case CELL_TYPE_MOSS:
                        PlaceMoss(position);
                        break;
                }
            }
        }
    }
}
