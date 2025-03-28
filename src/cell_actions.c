#include "cell_actions.h"
#include "grid.h"
#include "cell_types.h"
#include "cell_defaults.h"  // Add this include
#include <stdio.h>

// Place soil at the given position
void PlaceSoil(Vector2 position) {
    int x = (int)position.x;
    int y = (int)position.y;
    
    // Ensure position is within grid bounds
    if(x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) return;
    
    // Initialize with defaults first
    InitializeCellDefaults(&grid[y][x], CELL_TYPE_SOIL);
    grid[y][x].position = (Vector2){x * CELL_SIZE, y * CELL_SIZE};
}

// Place water at the given position
void PlaceWater(Vector2 position) {
    int x = (int)position.x;
    int y = (int)position.y;
    
    // Ensure position is within grid bounds
    if(x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) return;
    
    // Initialize with defaults first
    InitializeCellDefaults(&grid[y][x], CELL_TYPE_WATER);
    // Give newly placed water a random moisture level between 700 and 1000
    grid[y][x].moisture = 700 + GetRandomValue(0, 300);
    grid[y][x].position = (Vector2){x * CELL_SIZE, y * CELL_SIZE};
    
    // Update color based on moisture
    float intensityPct = (float)grid[y][x].moisture / 1000.0f;
    grid[y][x].baseColor = (Color){
        0 + (int)(200 * (1.0f - intensityPct)),
        120 + (int)(135 * (1.0f - intensityPct)),
        255,
        255
    };
}

// Place rock at the given position
void PlaceRock(Vector2 position) {
    int x = (int)position.x;
    int y = (int)position.y;
    
    // Ensure position is within grid bounds
    if(x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) return;
    
    // Initialize with defaults first
    InitializeCellDefaults(&grid[y][x], CELL_TYPE_ROCK);
    grid[y][x].position = (Vector2){x * CELL_SIZE, y * CELL_SIZE};
    
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
    if(x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) return;
    
    // Only allow plants to grow on soil
    if(grid[y][x].type != CELL_TYPE_SOIL && grid[y][x].type != CELL_TYPE_AIR) {
        return;
    }
    
    // Initialize with defaults first
    InitializeCellDefaults(&grid[y][x], CELL_TYPE_PLANT);
    grid[y][x].position = (Vector2){x * CELL_SIZE, y * CELL_SIZE};
    
    // Add some color variation to plants
    int greenVariation = GetRandomValue(-20, 20);
    grid[y][x].baseColor = (Color){
        20 + GetRandomValue(0, 30),         // Small amount of red
        150 + greenVariation,               // Varied green
        40 + GetRandomValue(-20, 20),       // Small amount of blue
        255
    };
    
    // Start with some energy for growth
    grid[y][x].Energy = 5 + GetRandomValue(0, 5);
    
    // Initialize age (starts at 0)
    grid[y][x].age = 0;
    
    // Plants start with moderate moisture needs
    grid[y][x].moisture = 50 + GetRandomValue(-10, 10);
    
    // Set a unique ID if tracking plants individually
    static int nextPlantID = 1;
    grid[y][x].objectID = nextPlantID++;
}

// Place moss at the given position
void PlaceMoss(Vector2 position) {
    int x = (int)position.x;
    int y = (int)position.y;
    
    // Ensure position is within grid bounds
    if(x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) return;
    
    // Initialize with defaults first
    InitializeCellDefaults(&grid[y][x], CELL_TYPE_MOSS);
    grid[y][x].position = (Vector2){x * CELL_SIZE, y * CELL_SIZE};
    
    // Moss has a darker green shade with some variation
    int greenVariation = GetRandomValue(-10, 10);
    grid[y][x].baseColor = (Color){
        10 + GetRandomValue(0, 10),        // Almost no red
        80 + greenVariation,               // Dark green with variation
        30 + GetRandomValue(-10, 10),      // Small amount of blue
        255
    };
    
    // Moss starts with less energy than plants
    grid[y][x].Energy = 3 + GetRandomValue(0, 3);
    
    // Initialize age (starts at 0)
    grid[y][x].age = 0;
    
    // Moss prefers higher moisture
    grid[y][x].moisture = 70 + GetRandomValue(-5, 15);
}

// Place air at the given position
void PlaceAir(Vector2 position) {
    int x = (int)position.x;
    int y = (int)position.y;
    
    // Ensure position is within grid bounds
    if(x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) return;
    
    // Initialize with defaults first
    InitializeCellDefaults(&grid[y][x], CELL_TYPE_AIR);
    grid[y][x].position = (Vector2){x * CELL_SIZE, y * CELL_SIZE};
    
    // Air can have slight moisture variation
    grid[y][x].moisture = GetRandomValue(5, 15);
    
    // Update color based on moisture (invisible until high moisture)
    if (grid[y][x].moisture > 75) {
        int brightness = (grid[y][x].moisture - 75) * (255 / 25);
        grid[y][x].baseColor = (Color){brightness, brightness, brightness, 255};
    } else {
        grid[y][x].baseColor = BLACK;  // Invisible air
    }
}

// Move cell function - swaps properties but not position of two cells
void MoveCell(int x1, int y1, int x2, int y2) {
    // Bounds checking to prevent memory corruption
    if (x1 < 0 || x1 >= GRID_WIDTH || y1 < 0 || y1 >= GRID_HEIGHT ||
        x2 < 0 || x2 >= GRID_WIDTH || y2 < 0 || y2 >= GRID_HEIGHT) {
        return;  // Skip if out of bounds
    }
    
    // Swap cells
    GridCell temp = grid[y1][x1];
    grid[y1][x1] = grid[y2][x2];
    grid[y2][x2] = temp;
    
    // Update position properties to match new grid locations
  //  grid[y1][x1].position = (Vector2){x1 * CELL_SIZE, y1 * CELL_SIZE};
  //  grid[y2][x2].position = (Vector2){x2 * CELL_SIZE, y2 * CELL_SIZE};
}

// Place cells in a circular pattern
void PlaceCircularPattern(int centerX, int centerY, int cellType, int radius) {
    for(int y = centerY - radius; y <= centerY + radius; y++) {
        for(int x = centerX - radius; x <= centerX + radius; x++) {
            float distanceSquared = (x - centerX) * (x - centerX) + (y - centerY) * (y - centerY);
            
            if(distanceSquared <= radius * radius && x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT) {
                switch(cellType) {
                    case CELL_TYPE_SOIL:
                        PlaceSoil((Vector2){x, y});
                        break;
                    case CELL_TYPE_WATER:
                        PlaceWater((Vector2){x, y});
                        break;
                    case CELL_TYPE_PLANT:
                        PlacePlant((Vector2){x, y});
                        break;
                    case CELL_TYPE_ROCK:
                        PlaceRock((Vector2){x, y});
                        break;
                    case CELL_TYPE_MOSS:
                        PlaceMoss((Vector2){x, y});
                        break;
                    case CELL_TYPE_AIR:
                        PlaceAir((Vector2){x, y});
                        break;
                }
            }
        }
    }
}

// Function to absorb moisture from one cell to another
// sourceMoisture is the source cell's moisture level
// targetMoisture is the cell absorbing the moisture
