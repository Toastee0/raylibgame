#include "cell_actions.h"
#include "grid.h"
#include "cell_types.h"
#include "cell_defaults.h"  // Add this include
#include <stdio.h>
#include <stdbool.h>

// Move cell contents from one position to another
void MoveCell(int fromX, int fromY, int toX, int toY) {
    // Check bounds
    if (fromX < 0 || fromX >= GRID_WIDTH || fromY < 0 || fromY >= GRID_HEIGHT ||
        toX < 0 || toX >= GRID_WIDTH || toY < 0 || toY >= GRID_HEIGHT) {
        return;
    }
    
    // Store destination cell properties before overwriting
    int destType = grid[toY][toX].type;
    int destMoisture = grid[toY][toX].moisture;
    int destTemp = grid[toY][toX].temperature;
    int destAge = grid[toY][toX].age;
    Color destColor = grid[toY][toX].baseColor;
    int destEnergy = grid[toY][toX].Energy;
    int destObjectID = grid[toY][toX].objectID;
    
    // Copy properties from source to destination
    grid[toY][toX].type = grid[fromY][fromX].type;
    grid[toY][toX].moisture = grid[fromY][fromX].moisture;
    grid[toY][toX].temperature = grid[fromY][fromX].temperature;
    grid[toY][toX].age = grid[fromY][fromX].age;
    grid[toY][toX].baseColor = grid[fromY][fromX].baseColor;
    grid[toY][toX].Energy = grid[fromY][fromX].Energy;
    grid[toY][toX].objectID = grid[fromY][fromX].objectID;
    
    // Source becomes the destination's previous type (not always air)
    grid[fromY][fromX].type = destType;
    grid[fromY][fromX].moisture = destMoisture;
    grid[fromY][fromX].temperature = destTemp;
    grid[fromY][fromX].age = destAge;
    grid[fromY][fromX].baseColor = destColor;
    grid[fromY][fromX].Energy = destEnergy;
    grid[fromY][fromX].objectID = destObjectID;
}

// Swap the contents of two cells
void SwapCells(int x1, int y1, int x2, int y2) {
    // Check bounds
    if (x1 < 0 || x1 >= GRID_WIDTH || y1 < 0 || y1 >= GRID_HEIGHT ||
        x2 < 0 || x2 >= GRID_WIDTH || y2 < 0 || y2 >= GRID_HEIGHT) {
        return;
    }
    
    // Temporarily store the properties of the first cell
    GridCell temp;
    temp.type = grid[y1][x1].type;
    temp.moisture = grid[y1][x1].moisture;
    temp.temperature = grid[y1][x1].temperature;
    temp.age = grid[y1][x1].age;
    temp.baseColor = grid[y1][x1].baseColor;
    temp.Energy = grid[y1][x1].Energy;
    temp.is_falling = grid[y1][x1].is_falling;
    temp.updated_this_frame = grid[y1][x1].updated_this_frame;
    temp.objectID = grid[y1][x1].objectID;
    
    // Copy properties from second cell to first cell
    grid[y1][x1].type = grid[y2][x2].type;
    grid[y1][x1].moisture = grid[y2][x2].moisture;
    grid[y1][x1].temperature = grid[y2][x2].temperature;
    grid[y1][x1].age = grid[y2][x2].age;
    grid[y1][x1].baseColor = grid[y2][x2].baseColor;
    grid[y1][x1].Energy = grid[y2][x2].Energy;
    grid[y1][x1].is_falling = grid[y2][x2].is_falling;
    grid[y1][x1].updated_this_frame = grid[y2][x2].updated_this_frame;
    grid[y1][x1].objectID = grid[y2][x2].objectID;
    
    // Copy properties from temporary storage to second cell
    grid[y2][x2].type = temp.type;
    grid[y2][x2].moisture = temp.moisture;
    grid[y2][x2].temperature = temp.temperature;
    grid[y2][x2].age = temp.age;
    grid[y2][x2].baseColor = temp.baseColor;
    grid[y2][x2].Energy = temp.Energy;
    grid[y2][x2].is_falling = temp.is_falling;
    grid[y2][x2].updated_this_frame = temp.updated_this_frame;
    grid[y2][x2].objectID = temp.objectID;
}

// Place soil at the given position
void PlaceSoil(Vector2 position) {
    int x = (int)position.x;
    int y = (int)position.y;
    
    // Ensure position is within grid bounds
    if(x < 0 || x > GRID_WIDTH - 1 || y < 0 || y > GRID_HEIGHT - 1) return;
    
    // Initialize with defaults first
    InitializeCellDefaults(&grid[y][x], CELL_TYPE_SOIL);
    grid[y][x].position = (Vector2){x * CELL_SIZE, y * CELL_SIZE};
}

// Place water at the given position
void PlaceWater(Vector2 position) {
    int x = (int)position.x;
    int y = (int)position.y;
    
    // Ensure position is within grid bounds
    if(x < 0 || x > GRID_WIDTH - 1 || y < 0 || y > GRID_HEIGHT - 1) return;
    
    // Initialize with defaults first
    InitializeCellDefaults(&grid[y][x], CELL_TYPE_WATER);
    // Give newly placed water a random moisture level between 70 and 100
    grid[y][x].moisture = 70 + GetRandomValue(0, 30);
    grid[y][x].position = (Vector2){x * CELL_SIZE, y * CELL_SIZE};
    
    // Update color based on moisture
    float intensityPct = (float)grid[y][x].moisture / 100.0f;
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
    if(x < 0 || x > GRID_WIDTH - 1 || y < 0 || y > GRID_HEIGHT - 1) return;
    
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
    if(x < 0 || x > GRID_WIDTH - 1 || y < 0 || y > GRID_HEIGHT - 1) return;
    
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
    if(x < 0 || x > GRID_WIDTH - 1 || y < 0 || y > GRID_HEIGHT - 1) return;
    
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
    if(x < 0 || x > GRID_WIDTH - 1 || y < 0 || y > GRID_HEIGHT - 1) return;
    
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

// Place cells in a circular pattern
void PlaceCircularPattern(int centerX, int centerY, int cellType, int radius) {
    for(int y = centerY - radius; y <= centerY + radius; y++) {
        for(int x = centerX - radius; x <= centerX + radius; x++) {
            // Skip border cells
            if (x <= 0 || x > GRID_WIDTH - 1 || y <= 0 || y > GRID_HEIGHT - 1) {
                continue;
            }

            float distanceSquared = (x - centerX) * (x - centerX) + (y - centerY) * (y - centerY);

            if(distanceSquared <= radius * radius && x > 0 && x < GRID_WIDTH && y > 0 && y < GRID_HEIGHT) {
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
