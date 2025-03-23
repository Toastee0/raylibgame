#include "cell_actions.h"
#include "grid.h"
#include "cell_types.h"
#include <stdio.h>

// Place soil at the given position
void PlaceSoil(Vector2 position) {
    int x = (int)position.x;
    int y = (int)position.y;
    
    // Ensure position is within grid bounds
    if(x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) return;
    
    grid[y][x].type = CELL_TYPE_SOIL;
    grid[y][x].baseColor = BROWN;
    grid[y][x].moisture = 20;  // Start sand with 20 moisture
    grid[y][x].position = (Vector2){x * CELL_SIZE, y * CELL_SIZE};
    grid[y][x].is_falling = false;
}

// Place water at the given position
void PlaceWater(Vector2 position) {
    int x = (int)position.x;
    int y = (int)position.y;
    
    // Ensure position is within grid bounds
    if(x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) return;
    
    // Give newly placed water a random moisture level between 70 and 100
    int randomMoisture = 70 + GetRandomValue(0, 30);
    
    grid[y][x].type = CELL_TYPE_WATER;
    grid[y][x].moisture = randomMoisture;
    
    // Set color based on moisture/density
    float intensityPct = (float)randomMoisture / 100.0f;
    grid[y][x].baseColor = (Color){
        0 + (int)(200 * (1.0f - intensityPct)),
        120 + (int)(135 * (1.0f - intensityPct)),
        255,
        255
    };
    
    grid[y][x].position = (Vector2){x * CELL_SIZE, y * CELL_SIZE};
}


// Place cells in a circular pattern
void PlaceCircularPattern(int centerX, int centerY, int cellType, int radius) {
    // Iterate through a square area and check if points are within the circle
    for(int y = centerY - radius; y <= centerY + radius; y++) {
        for(int x = centerX - radius; x <= centerX + radius; x++) {
            // Calculate distance from center
            float distanceSquared = (x - centerX) * (x - centerX) + (y - centerY) * (y - centerY);
            
            // If within radius and within grid bounds
            if(distanceSquared <= radius * radius && x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT) {
                // Place the appropriate cell type
                switch(cellType) {
                    case CELL_TYPE_SOIL:
                        PlaceSoil((Vector2){x, y});
                        break;
                    case CELL_TYPE_WATER:
                        PlaceWater((Vector2){x, y});
                        break;
                    case CELL_TYPE_PLANT:
                        if(grid[y][x].type == CELL_TYPE_AIR) { // Only place plant on empty cells
                            grid[y][x].type = CELL_TYPE_PLANT;
                            grid[y][x].baseColor = GREEN;
                            grid[y][x].position = (Vector2){x * CELL_SIZE, y * CELL_SIZE};
                        }
                        break;
                }
            }
        }
    }
}

// Move a cell from one position to another
void MoveCell(int x, int y, int x2, int y2) {
    // Add stronger boundary checks for screen edges

    // Special handling for sand falling through water
    if(grid[y][x].type == CELL_TYPE_SOIL && grid[y2][x2].type == CELL_TYPE_WATER) {
        // Save the total moisture before any operations
        int totalMoistureBefore = grid[y][x].moisture + grid[y2][x2].moisture;
        
        // Calculate how much moisture sand can absorb
        int availableMoisture = grid[y2][x2].moisture;
        int sandCurrentMoisture = grid[y][x].moisture;
        int sandCapacity = 100 - sandCurrentMoisture;
        
        // Determine how much sand will absorb
        int amountToAbsorb = (sandCapacity > availableMoisture) ? 
                                availableMoisture : sandCapacity;
        
        // Calculate remaining moisture
        int remainingMoisture = availableMoisture - amountToAbsorb;
        
        // If there's significant moisture left, we'll swap with modified values
        if(remainingMoisture > 10) {
            // Save original sand cell
            GridCell sandCell = grid[y][x];
            
            // Add absorbed moisture to sand 
            sandCell.moisture += amountToAbsorb;
            sandCell.is_falling = true;
            
            // Move sand to lower position
            grid[y2][x2] = sandCell;
            grid[y2][x2].position = (Vector2){x2 * CELL_SIZE, y2 * CELL_SIZE};
            
            // Remaining moisture becomes water in the upper position
            grid[y][x].type = CELL_TYPE_WATER;
            grid[y][x].moisture = remainingMoisture;
            grid[y][x].is_falling = false;
            
            // Update water color
            float intensity = (float)remainingMoisture / 100.0f;
            grid[y][x].baseColor = (Color){
                0 + (int)(200 * (1.0f - intensity)),
                120 + (int)(135 * (1.0f - intensity)),
                255,
                255
            };
        }
        else {
            // Move sand down, replacing water
            grid[y2][x2] = grid[y][x];
            grid[y2][x2].moisture += availableMoisture;  // Add all available moisture
            grid[y2][x2].is_falling = true;
            grid[y2][x2].position = (Vector2){x2 * CELL_SIZE, y2 * CELL_SIZE};
            
            // Clear the original cell
            grid[y][x].type = CELL_TYPE_AIR;
            grid[y][x].baseColor = BLACK;
            grid[y][x].moisture = 0;
            grid[y][x].is_falling = false;
        }

        // Strict conservation check - ensure total moisture is preserved
        int totalMoistureAfter = (grid[y][x].type == CELL_TYPE_AIR ? 0 : grid[y][x].moisture) + grid[y2][x2].moisture;
        
        if(totalMoistureAfter != totalMoistureBefore) {
            // Fix any conservation errors by adjusting the cell with more moisture
            if(grid[y][x].type != CELL_TYPE_AIR && grid[y][x].moisture >= grid[y2][x2].moisture) {
                grid[y][x].moisture = totalMoistureBefore - grid[y2][x2].moisture;
            } else {
                grid[y2][x2].moisture = totalMoistureBefore - (grid[y][x].type == CELL_TYPE_AIR ? 0 : grid[y][x].moisture);
            }
        }
        
        // Update sand color based on new moisture
        float intensity = (float)grid[y2][x2].moisture / 100.0f;
        grid[y2][x2].baseColor = (Color){
            127 - (intensity * 51),
            106 - (intensity * 43),
            79 - (intensity * 32),
            255
        };
        
        return;
    }
    
    // Special handling for sand falling through vapor
    if(grid[y][x].type == CELL_TYPE_SOIL && grid[y2][x2].type == CELL_TYPE_AIR) {
        // Save the total moisture before any operations
        int totalMoistureBefore = grid[y][x].moisture + grid[y2][x2].moisture;
        
        // Calculate how much moisture sand can absorb
        int availableMoisture = grid[y2][x2].moisture;
        int sandCurrentMoisture = grid[y][x].moisture;
        int sandCapacity = 100 - sandCurrentMoisture;
        
        // SPECIAL CASE: If sand is already fully saturated (100% moisture)
        if(sandCapacity <= 0) {
            // All vapor moisture gets pushed upward as sand falls down
            
            // Move sand down (without adding more moisture since it's already full)
            grid[y2][x2] = grid[y][x];
            grid[y2][x2].is_falling = true;
            grid[y2][x2].position = (Vector2){x2 * CELL_SIZE, y2 * CELL_SIZE};
            
            // All vapor's moisture moves to the upper cell (now vapor)
            grid[y][x].type = CELL_TYPE_AIR;
            grid[y][x].moisture = availableMoisture;
            grid[y][x].is_falling = false;
            
            // Update vapor color based on moisture
            if(availableMoisture < 50) {
                grid[y][x].baseColor = BLACK; // Invisible
            } else {
                int brightness = 128 + (int)(127 * ((float)(availableMoisture - 50) / 50.0f));
                grid[y][x].baseColor = (Color){
                    brightness, brightness, brightness, 255
                };
            }
            
            // Verify moisture conservation 
            int totalMoistureAfter = grid[y][x].moisture + grid[y2][x2].moisture;
            if(totalMoistureAfter != totalMoistureBefore) {
                // Fix any conservation errors
                grid[y][x].moisture = totalMoistureBefore - grid[y2][x2].moisture;
            }
            
            return;
        }
        
        // Continue with existing logic for non-saturated sand
        // Determine how much sand will absorb
        int amountToAbsorb = (sandCapacity > availableMoisture) ? 
                                availableMoisture : sandCapacity;
        
        // Calculate remaining moisture
        int remainingMoisture = availableMoisture - amountToAbsorb;
        
        // If there's significant moisture left, we'll swap with modified values
        if(remainingMoisture > 10) {
            // Save original sand cell
            GridCell sandCell = grid[y][x];
            
            // Add absorbed moisture to sand 
            sandCell.moisture += amountToAbsorb;
            sandCell.is_falling = true;
            
            // Move sand to lower position
            grid[y2][x2] = sandCell;
            grid[y2][x2].position = (Vector2){x2 * CELL_SIZE, y2 * CELL_SIZE};
            
            // Remaining moisture becomes vapor in the upper position
            grid[y][x].type = CELL_TYPE_AIR;
            grid[y][x].moisture = remainingMoisture;
            grid[y][x].is_falling = false;
            
            // Update vapor color
            if(remainingMoisture < 50) {
                grid[y][x].baseColor = BLACK; // Invisible
            } else {
                int brightness = 128 + (int)(127 * ((float)(remainingMoisture - 50) / 50.0f));
                grid[y][x].baseColor = (Color){
                    brightness, brightness, brightness, 255
                };
            }
        }
        else {
            // Move sand down, replacing vapor
            grid[y2][x2] = grid[y][x];
            grid[y2][x2].moisture += availableMoisture;  // Add all available moisture
            grid[y2][x2].is_falling = true;
            grid[y2][x2].position = (Vector2){x2 * CELL_SIZE, y2 * CELL_SIZE};
            
            // Clear the original cell
            grid[y][x].type = CELL_TYPE_AIR;
            grid[y][x].baseColor = BLACK;
            grid[y][x].moisture = 0;
            grid[y][x].is_falling = false;
        }

        // Strict conservation check - ensure total moisture is preserved
        int totalMoistureAfter = (grid[y][x].type == CELL_TYPE_AIR ? 0 : grid[y][x].moisture) + grid[y2][x2].moisture;
        
        if(totalMoistureAfter != totalMoistureBefore) {
            // Fix any conservation errors by adjusting the cell with more moisture
            if(grid[y][x].type != CELL_TYPE_AIR && grid[y][x].moisture >= grid[y2][x2].moisture) {
                grid[y][x].moisture = totalMoistureBefore - grid[y2][x2].moisture;
            } else {
                grid[y2][x2].moisture = totalMoistureBefore - (grid[y][x].type == CELL_TYPE_AIR ? 0 : grid[y][x].moisture);
            }
        }
        
        // Update sand color based on new moisture
        float intensity = (float)grid[y2][x2].moisture / 100.0f;
        grid[y2][x2].baseColor = (Color){
            127 - (intensity * 51),
            106 - (intensity * 43),
            79 - (intensity * 32),
            255
        };
        
        return;
    }
    
    // Special handling for water falling through vapor
    if(grid[y][x].type == CELL_TYPE_WATER && grid[y2][x2].type == CELL_TYPE_AIR) {
        // Swap the cells - water falls through vapor
        GridCell temp = grid[y2][x2];
        grid[y2][x2] = grid[y][x];
        grid[y][x] = temp;
        
        // Preserve falling state after swap
        grid[y2][x2].is_falling = true;
        
        // Update positions for both cells
        grid[y2][x2].position = (Vector2){x2 * CELL_SIZE, y2 * CELL_SIZE};
        grid[y][x].position = (Vector2){x * CELL_SIZE, y * CELL_SIZE};
        
        return;
    }
    
    // Special handling for vapor rising through less dense vapor
    if(grid[y][x].type == CELL_TYPE_AIR && grid[y2][x2].type == CELL_TYPE_AIR && grid[y][x].moisture > grid[y2][x2].moisture) {
        // Swap positions - denser vapor displaces less dense vapor when rising
        GridCell temp = grid[y2][x2];
        grid[y2][x2] = grid[y][x];
        grid[y][x] = temp;
        
        // Update positions for both cells
        grid[y2][x2].position = (Vector2){x2 * CELL_SIZE, y2 * CELL_SIZE};
        grid[y][x].position = (Vector2){x * CELL_SIZE, y * CELL_SIZE};
        
        return;
    }
    
    // Standard case - move cell from (x,y) to (x2,y2)
    grid[y2][x2] = grid[y][x];
    
    // Preserve falling state for the moved cell
    grid[y2][x2].is_falling = true;
    
    // Update the position property of the moved cell
    grid[y2][x2].position = (Vector2){x2 * CELL_SIZE, y2 * CELL_SIZE};
    
    // Reset the source cell
    grid[y][x].type = CELL_TYPE_AIR;
    grid[y][x].baseColor = BLACK;
    grid[y][x].moisture = 0;
    grid[y][x].is_falling = false;
}
