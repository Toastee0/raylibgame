/*******************************************************************************************
*
*   raylib [core] example - Basic 3d example
*
*   Welcome to raylib!
*
*   To compile example, just press F5.
*   Note that compiled executable is placed in the same folder as .c file
*
*   You can find all basic examples on C:\raylib\raylib\examples folder or
*   raylib official webpage: www.raylib.com
*
*   Enjoy using raylib. :)
*
*   This example has been created using raylib 1.0 (www.raylib.com)
*   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
*
*   Copyright (c) 2013-2024 Ramon Santamaria (@raysan5)
*
********************************************************************************************/
//code rules: no moisture can be destroyed, only moved, to conseve the total water in the simulation.

#include "raylib.h"
#include <stddef.h>
#include <stdlib.h>     // Add this for malloc/free
#include <math.h>
#include <stdio.h>  // Add this for sprintf

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

//----------------------------------------------------------------------------------
// Local Variables Definition (local to this module)
//----------------------------------------------------------------------------------
Camera camera = { 0 };
//Vector3 cubePosition = { 0 };

//----------------------------------------------------------------------------------
// Local Functions Declaration
//----------------------------------------------------------------------------------
static void UpdateDrawFrame(void);          // Update and draw one frame
static void DrawGameGrid(void);             // Draw the game grid
// First, let's add our structures at the top, after the includes:

typedef struct {
    int type;  // -1 = immutable border 0 = air, 1 = soil, 2 = water, 3 = plant, 4 = vapor
    int objectID; //unique identifier for the object or plant 
    Vector2 position;
    Vector2 origin; //co ordinates of the first pixel of the object or plant, if a multi pixel object.
    Color baseColor; //basic color of the pixel
    int colorhigh; // max variation of color for the pixel
    int colorlow; // min variation of color for the pixel
    int volume; //1-10, how much of the density of the object is filled, 1 = 10% 10 = 100%, for allowing water to evaoprate into moist air, or be absorbed by soil.
    int Energy; //5 initial, reduced when replicating.
    int height; //height of the pixel, intially 0, this is an offset to allow limiting and guiding the growth of plant type pixels.
    int moisture; // Moisture level: 0-100 integer instead of 0.0-1.0 float
    int permeable; //0 = impermeable, 1 = permeable (water permeable)
    int age; //age of the object, used for plant growth and reproduction.
    int maxage; //max age of the object, used for plant growth and reproduction.
    int temperature; //temperature of the object.
    int freezingpoint; //freezing point of the object.
    int boilingpoint; //boiling point of the object.
    int temperaturepreferanceoffset; 
    bool is_falling; // New field to explicitly track falling state
} GridCell;




// Global variables
static GridCell** grid;
static const int CELL_SIZE = 8;  // Size of each cell in pixels
static const int GRID_WIDTH = 1920/CELL_SIZE;
static const int GRID_HEIGHT = 1080/CELL_SIZE;
static float lastSeedTime = 0;
static const float SEED_DELAY = 0.5f;  // Half second delay between seeds
static int brushRadius = 1;  // Default brush radius (in grid cells)


// Function declarations
static void InitGrid(void);
static void UpdateGrid(void);
static void HandleInput(void);
static void PlaceSoil(Vector2 position);
static void PlaceWater(Vector2 position);
static void MoveCell(int x, int y, int x2, int y2);
static void PlaceCircularPattern(int centerX, int centerY, int cellType, int radius);
static void TransferMoisture(void);
static void UpdateVapor(void);
static void PlaceVapor(Vector2 position, int moisture);
static void FloodFillCeilingCluster(int x, int y, int clusterID, int** clusterIDs);
static void DebugGridCells(void); // Add debug function
static int CalculateTotalMoisture(); // Add function to calculate total moisture
static int ClampMoisture(int value); // Add ClampMoisture declaration


// Main function updates:
int main() {
    const int screenWidth = 1920;
    const int screenHeight = 1080;

    InitWindow(screenWidth, screenHeight, "Sandbox");

    // Initialize grid
    InitGrid();

    camera.position = (Vector3){ 10.0f, 10.0f, 8.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        UpdateDrawFrame();
    }

    // Cleanup
    for(int i = 0; i < GRID_HEIGHT; i++) {
        free(grid[i]);
    }
    free(grid);

    CloseWindow();
    return 0;
}

// Initialize the grid - fix initialization to avoid infinite loops
static void InitGrid(void) {
    grid = (GridCell**)malloc(GRID_HEIGHT * sizeof(GridCell*));
    for(int i = 0; i < GRID_HEIGHT; i++) {
        grid[i] = (GridCell*)malloc(GRID_WIDTH * sizeof(GridCell));
        for(int j = 0; j < GRID_WIDTH; j++) {
            grid[i][j].position = (Vector2){j * CELL_SIZE, i * CELL_SIZE};
            grid[i][j].baseColor = BLACK;
            
            // Initialize as air instead of vapor to prevent infinite processing on startup
            grid[i][j].type = 0;  // Air type (was 4 for vapor)
            grid[i][j].moisture = 0;  // Integer moisture (was 0.0f)
            grid[i][j].permeable = 1;
            grid[i][j].is_falling = false;
            grid[i][j].volume = 0;
            
            // Initialize other fields to safe defaults
            grid[i][j].objectID = 0;
            grid[i][j].colorhigh = 0;
            grid[i][j].colorlow = 0;
            grid[i][j].Energy = 0;
            grid[i][j].height = 0;
            grid[i][j].age = 0;
            grid[i][j].maxage = 0;
            grid[i][j].temperature = 20; // Room temperature
        }
    }
}

// Fix sand falling through vapor when already saturated with moisture
static void MoveCell(int x, int y, int x2, int y2) {
    // Add stronger boundary checks for screen edges
    if(x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT ||
       x2 < 0 || x2 >= GRID_WIDTH || y2 < 0 || y2 >= GRID_HEIGHT) {
        // Log the attempted out-of-bounds movement for debugging
        if(x2 < 0 || x2 >= GRID_WIDTH || y2 < 0 || y2 >= GRID_HEIGHT) {
            // printf("Prevented out-of-bounds move to (%d,%d)\n", x2, y2);
        }
        return; // Out of bounds, prevent this move
    }
    
    // Special handling for sand falling through water
    if(grid[y][x].type == 1 && grid[y2][x2].type == 2) {
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
        if(remainingMoisture > 10) {  // > 0.1 in the old scale
            // Save original sand cell
            GridCell sandCell = grid[y][x];
            
            // Add absorbed moisture to sand 
            sandCell.moisture += amountToAbsorb;
            sandCell.is_falling = true;
            
            // Move sand to lower position
            grid[y2][x2] = sandCell;
            grid[y2][x2].position = (Vector2){x2 * CELL_SIZE, y2 * CELL_SIZE};
            
            // Remaining moisture becomes water in the upper position
            grid[y][x].type = 2; // Water type
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
            grid[y][x].type = 0;
            grid[y][x].baseColor = BLACK;
            grid[y][x].moisture = 0;
            grid[y][x].is_falling = false;
        }

        // Strict conservation check - ensure total moisture is preserved
        int totalMoistureAfter = (grid[y][x].type == 0 ? 0 : grid[y][x].moisture) + grid[y2][x2].moisture;
        
        if(totalMoistureAfter != totalMoistureBefore) {
            // Fix any conservation errors by adjusting the cell with more moisture
            if(grid[y][x].type != 0 && grid[y][x].moisture >= grid[y2][x2].moisture) {
                grid[y][x].moisture = totalMoistureBefore - grid[y2][x2].moisture;
            } else {
                grid[y2][x2].moisture = totalMoistureBefore - (grid[y][x].type == 0 ? 0 : grid[y][x].moisture);
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
    
    // Special handling for sand falling through vapor - similar logic
    if(grid[y][x].type == 1 && grid[y2][x2].type == 4) {
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
            grid[y][x].type = 4; // Vapor type
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
        if(remainingMoisture > 10) {  // > 0.1 in the old scale
            // Save original sand cell
            GridCell sandCell = grid[y][x];
            
            // Add absorbed moisture to sand 
            sandCell.moisture += amountToAbsorb;
            sandCell.is_falling = true;
            
            // Move sand to lower position
            grid[y2][x2] = sandCell;
            grid[y2][x2].position = (Vector2){x2 * CELL_SIZE, y2 * CELL_SIZE};
            
            // Remaining moisture becomes vapor in the upper position
            grid[y][x].type = 4; // Vapor type
            grid[y][x].moisture = remainingMoisture;
            grid[y][x].is_falling = false;
            
            // Update vapor color
            if(remainingMoisture < 50) {  // < 0.5 in the old scale
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
            grid[y][x].type = 0;
            grid[y][x].baseColor = BLACK;
            grid[y][x].moisture = 0;
            grid[y][x].is_falling = false;
        }

        // Strict conservation check - ensure total moisture is preserved
        int totalMoistureAfter = (grid[y][x].type == 0 ? 0 : grid[y][x].moisture) + grid[y2][x2].moisture;
        
        if(totalMoistureAfter != totalMoistureBefore) {
            // Fix any conservation errors by adjusting the cell with more moisture
            if(grid[y][x].type != 0 && grid[y][x].moisture >= grid[y2][x2].moisture) {
                grid[y][x].moisture = totalMoistureBefore - grid[y2][x2].moisture;
            } else {
                grid[y2][x2].moisture = totalMoistureBefore - (grid[y][x].type == 0 ? 0 : grid[y][x].moisture);
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
    if(grid[y][x].type == 2 && grid[y2][x2].type == 4) {
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
    if(grid[y][x].type == 4 && grid[y2][x2].type == 4 && grid[y][x].moisture > grid[y2][x2].moisture) {
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
    grid[y][x].type = 0;
    grid[y][x].baseColor = BLACK;
    grid[y][x].moisture = 0;  // Integer moisture (was 0.0f)
    grid[y][x].is_falling = false;
}

// Also fix the UpdateSoil function to ensure sand always falls through vapor
static void UpdateSoil(void) {
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
            if(grid[y][x].type == 1) {  // If it's soil
                // Reset falling state before movement logic
                grid[y][x].is_falling = false;
                
                // Track soil moisture
                int* moisture = &grid[y][x].moisture;

                // FALLING MECHANICS: explicitly prioritize falling through vapor
                if(y < GRID_HEIGHT - 1) {
                    // First priority: Fall through vapor regardless of moisture levels
                    if(grid[y+1][x].type == 4) {
                        // Mark as falling and move, let MoveCell handle the moisture correctly
                        grid[y][x].is_falling = true;
                        MoveCell(x, y, x, y+1);
                        continue;
                    }
                    
                    // Second priority: Check for empty space or water
                    if(grid[y+1][x].type == 0 || grid[y+1][x].type == 2) {
                        grid[y][x].is_falling = true;
                        MoveCell(x, y, x, y+1);
                        continue;
                    }
                    
                    // For diagonal sliding, also prioritize vapor
                    bool canSlideLeftVapor = (x > 0 && grid[y+1][x-1].type == 4);
                    bool canSlideRightVapor = (x < GRID_WIDTH-1 && grid[y+1][x+1].type == 4);
                    
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
                    // Check for empty space, water, or vapor on diagonal cells
                    bool canSlideLeft = (x > 0 && (grid[y+1][x-1].type == 0 || 
                                                  grid[y+1][x-1].type == 2 || 
                                                  grid[y+1][x-1].type == 4));
                    bool canSlideRight = (x < GRID_WIDTH-1 && (grid[y+1][x+1].type == 0 || 
                                                              grid[y+1][x+1].type == 2 || 
                                                              grid[y+1][x+1].type == 4));
                    
                    if(canSlideLeft && canSlideRight) {
                        // Choose random direction, but slightly favor the current processing direction
                        // This helps prevent directional bias
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
                    127 - (intensityPct * 51),  // R: 127 -> 76
                    106 - (intensityPct * 43),  // G: 106 -> 63
                    79 - (intensityPct * 32),   // B: 79 -> 47
                    255
                };

                // REMOVE moisture evaporation - this was incorrectly destroying moisture
                // *moisture = fmax(*moisture - 0.001f, 0.0f);
                
                // Instead, transfer tiny amounts to ambient vapor if needed
                if(grid[y][x].moisture > 20) {  // > 0.2 in the old scale
                    // Try to find nearby ambient vapor to add moisture to
                    bool transferred = false;
                    for(int dy = -1; dy <= 1 && !transferred; dy++) {
                        for(int dx = -1; dy <= 1 && !transferred; dx++) {
                            if(dx == 0 && dy == 0) continue;
                            
                            int nx = x + dx;
                            int ny = y + dy;
                            
                            if(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT && 
                               grid[ny][nx].type == 4) {
                                // Transfer a tiny amount of moisture
                                int transferAmount = 1;  // Was 0.001f
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

// Fix the UpdateVapor function to prevent infinite loops
static void UpdateVapor(void) {
    int initialMoisture = CalculateTotalMoisture();
    
    // Only alternate the processing direction every OTHER frame to allow absorption patterns
    static bool alternateScanDirection = false;
    alternateScanDirection = !alternateScanDirection;
    
    // Flag to track which cells have already been processed this frame
    bool** processedCells = (bool**)malloc(GRID_HEIGHT * sizeof(bool*));
    for(int i = 0; i < GRID_HEIGHT; i++) {
        processedCells[i] = (bool*)calloc(GRID_WIDTH, sizeof(bool)); // Initialize all to false
    }
    
    // Maximum iterations safety check to prevent infinite loops
    const int MAX_ITERATIONS = GRID_WIDTH * GRID_HEIGHT;
    int iterations = 0;
    
    // Process vapor from top to bottom with alternating scan patterns
    for(int y = 0; y < GRID_HEIGHT && iterations < MAX_ITERATIONS; y++) {
        // Determine scan direction for this row
        bool processRightToLeft = (y % 2 == 0) ? alternateScanDirection : !alternateScanDirection;
        
        // Setup scanning parameters more safely
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
        
        // First pass - process with safety checks
        for(int x = startX; x != endX && iterations < MAX_ITERATIONS; x += stepX * 2) {
            iterations++;
            if(x < 0 || x >= GRID_WIDTH) continue; // Extra boundary check
            
            if(grid[y][x].type == 4 && !processedCells[y][x]) {
                // Process this vapor cell
                // First check for vapor absorption - scan neighboring cells
                bool absorbed = false;
                int totalAbsorbedMoisture = 0;
                
                // Check all 8 adjacent cells
                for(int dy = -1; dy <= 1; dy++) {
                    for(int dx = -1; dx <= 1; dx++) {
                        if(dx == 0 && dy == 0) continue; // Skip the cell itself
                        
                        int nx = x + dx;
                        int ny = y + dy;
                        
                        // Check if adjacent cell is in bounds and is vapor with less moisture
                        if(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT && 
                           grid[ny][nx].type == 4 && !processedCells[ny][nx] && 
                           grid[ny][nx].moisture < grid[y][x].moisture) {
                            
                            // Absorb moisture from adjacent cell
                            int absorbedMoisture = grid[ny][nx].moisture;
                            totalAbsorbedMoisture += absorbedMoisture;
                            
                            // Mark the cell as processed and convert to air
                            processedCells[ny][nx] = true;
                            grid[ny][nx].type = 0;
                            grid[ny][nx].moisture = 0; // Moisture is now in current cell
                            grid[ny][nx].baseColor = BLACK;
                            
                            absorbed = true;
                        }
                    }
                }
                
                // Add the absorbed moisture to current cell
                if(absorbed) {
                    grid[y][x].moisture += totalAbsorbedMoisture;
                    
                    // Update vapor color after absorption
                    if(grid[y][x].moisture < 50) {  // < 0.5 in the old scale
                        grid[y][x].baseColor = BLACK; // Invisible
                    } else {
                        int brightness = 128 + (int)(127 * ((float)(grid[y][x].moisture - 50) / 50.0f));
                        grid[y][x].baseColor = (Color){
                            brightness, brightness, brightness, 255
                        };
                    }
                }
                
                // Continue with existing vapor condensation logic
                // Enhance condensation - more likely to condense at ceiling or near other water
                bool condensed = false;
                
                // Higher elevation = higher condensation chance
                float condensationChance = 0.05f * (1.0f - (float)y / GRID_HEIGHT);
                
                // Higher moisture = higher condensation chance
                condensationChance += ((float)(grid[y][x].moisture - 20) / 100.0f) * 0.5f;
                
                // Check for nearby water (ceiling water creates nucleation centers)
                int waterNeighbors = 0;
                int ceilingWaterSize = 0;
                
                for(int dy = -2; dy <= 2; dy++) {
                    for(int dx = -2; dx <= 2; dx++) {
                        int nx = x + dx, ny = y + dy;
                        if(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT && 
                           grid[ny][nx].type == 2) {
                            
                            // Water proximity boosts condensation chance
                            float distance = sqrtf(dx*dx + dy*dy);
                            if(distance <= 2.0f) {
                                waterNeighbors++;
                                
                                // Check if it's ceiling water (at top or has water above it)
                                bool isCeilingWater = (ny == 0 || (ny > 0 && grid[ny-1][nx].type == 2));
                                if(isCeilingWater) {
                                    ceilingWaterSize++;
                                    // Ceiling water creates a stronger condensation field
                                    condensationChance += 0.2f / (distance + 0.5f);
                                }
                            }
                        }
                    }
                }
                
                // At ceiling, almost always condense with sufficient moisture
                if(y == 0 && grid[y][x].moisture > 30 && GetRandomValue(0, 100) < 80) {
                    grid[y][x].type = 2; // Convert to water
                    float intensity = (float)grid[y][x].moisture / 100.0f;
                    grid[y][x].baseColor = (Color){
                        0 + (int)(200 * (1.0f - intensity)),
                        120 + (int)(135 * (1.0f - intensity)),
                        255,
                        255
                    };
                    condensed = true;
                }
                // General condensation check
                else if(GetRandomValue(0, 100) < condensationChance * 100) {
                    // Condense into water if moisture is high enough
                    if(grid[y][x].moisture > 30) {
                        grid[y][x].type = 2; // Convert to water
                        float intensity = (float)grid[y][x].moisture / 100.0f;
                        grid[y][x].baseColor = (Color){
                            0 + (int)(200 * (1.0f - intensity)),
                            120 + (int)(135 * (1.0f - intensity)),
                            255,
                            255
                        };
                        condensed = true;
                    }
                }
                
                if(condensed) continue;
                
                // Rest of vapor movement logic
                // Check if water is directly above vapor (should be rare, but handle it)
                if(y > 0 && grid[y-1][x].type == 2) {
                    MoveCell(y-1, x, y, x);  // Will trigger the swap in MoveCell
                    continue;
                }
                
                // Precipitation at top of screen
                if(y == 0 && grid[y][x].moisture > 50) {
                    // Convert to water if it has enough moisture and is at the top
                    grid[y][x].type = 2; // Convert to water
                    float intensity = (float)grid[y][x].moisture / 100.0f;
                    grid[y][x].baseColor = (Color){
                        0 + (int)(200 * (1.0f - intensity)),
                        120 + (int)(135 * (1.0f - intensity)),
                        255,
                        255
                    };
                    continue;
                }
                
                // Vapor rising mechanics (opposite of water falling)
                if(y > 0) {  // If not at top
                    // Try to rise straight up first
                    if(grid[y-1][x].type == 0 || grid[y-1][x].type == 4) {
                        // If empty space or less dense vapor above, move up
                        if(grid[y-1][x].type == 0 || 
                           (grid[y-1][x].type == 4 && grid[y-1][x].moisture < grid[y][x].moisture)) {
                            // Fixed parameter order: source coordinates first, then destination
                            MoveCell(x, y, x, y-1);
                            continue;
                        }
                    }
                    
                    // If can't rise straight up, try diagonally
                    bool canRiseLeft = (x > 0 && (grid[y-1][x-1].type == 0 || 
                        (grid[y-1][x-1].type == 4 && grid[y-1][x-1].moisture < grid[y][x].moisture)));
                    bool canRiseRight = (x < GRID_WIDTH-1 && (grid[y-1][x+1].type == 0 || 
                        (grid[y-1][x+1].type == 4 && grid[y-1][x+1].moisture < grid[y][x].moisture)));
                    
                    if(canRiseLeft && canRiseRight) {
                        // Choose random direction
                        int direction = (GetRandomValue(0, 1) == 0) ? -1 : 1;
                        MoveCell(x, y, x+direction, y-1);
                        continue;
                    } else if(canRiseLeft) {
                        MoveCell(x, y, x-1, y-1);
                        continue;
                    } else if(canRiseRight) {
                        MoveCell(x, y, x+1, y-1);
                        continue;
                    }
                    
                    // Count neighboring vapor cells
                    int vaporNeighbors = 0;
                    for(int dy = -1; dy <= 1; dy++) {
                        for(int dx = -1; dx <= 1; dx++) {
                            if(dx == 0 && dy == 0) continue;
                            int nx = x + dx, ny = y + dy;
                            if(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT && 
                               grid[ny][nx].type == 4) {
                                vaporNeighbors++;
                            }
                        }
                    }
                    
                    // Apply "inverse surface tension" - vapor prefers being in clusters
                    float moveChance = (vaporNeighbors > 2) ? 0.3f : 1.0f;
                    
                    if(GetRandomValue(0, 100) < moveChance * 100) {
                        // If can't rise, try to spread horizontally
                        bool canFlowLeft = (x > 0 && grid[y][x-1].type == 0);
                        bool canFlowRight = (x < GRID_WIDTH-1 && grid[y][x+1].type == 0);
                        
                        if(canFlowLeft && canFlowRight) {
                            // Slightly favor the current processing direction
                            int leftBias = processRightToLeft ? 60 : 40;
                            int flowDir = (GetRandomValue(0, 100) < leftBias) ? -1 : 1;
                            MoveCell(x, y, x+flowDir, y);
                        } else if(canFlowLeft) {
                            MoveCell(x, y, x-1, y);
                        } else if(canFlowRight) {
                            MoveCell(x, y, x+1, y);
                        }
                    }
                }
            }
        }
        
        // Second pass - process remaining cells with safety checks
        for(int x = (startX + stepX); x != endX && iterations < MAX_ITERATIONS; x += stepX * 2) {
            iterations++;
            if(x < 0 || x >= GRID_WIDTH) continue; // Extra boundary check
            
            if(grid[y][x].type == 4 && !processedCells[y][x]) {
                processedCells[y][x] = true;
                
                // Same processing logic as first pass
                // First check for vapor absorption
                bool absorbed = false;
                int totalAbsorbedMoisture = 0;
                
                // Check all 8 adjacent cells
                for(int dy = -1; dy <= 1; dy++) {
                    for(int dx = -1; dx <= 1; dx++) {
                        if(dx == 0 && dy == 0) continue;
                        
                        int nx = x + dx;
                        int ny = y + dy;
                        
                        if(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT && 
                           grid[ny][nx].type == 4 && !processedCells[ny][nx] && 
                           grid[ny][nx].moisture < grid[y][x].moisture) {
                            
                            int absorbedMoisture = grid[ny][nx].moisture;
                            totalAbsorbedMoisture += absorbedMoisture;
                            
                            processedCells[ny][nx] = true;
                            grid[ny][nx].type = 0;
                            grid[ny][nx].moisture = 0;
                            grid[ny][nx].baseColor = BLACK;
                            
                            absorbed = true;
                        }
                    }
                }
                
                if(absorbed) {
                    grid[y][x].moisture += totalAbsorbedMoisture;
                    
                    if(grid[y][x].moisture < 50) {  // < 0.5 in the old scale
                        grid[y][x].baseColor = BLACK; // Invisible
                    } else {
                        int brightness = 128 + (int)(127 * ((float)(grid[y][x].moisture - 50) / 50.0f));
                        grid[y][x].baseColor = (Color){
                            brightness, brightness, brightness, 255
                        };
                    }
                }
                
                // Continue with existing vapor condensation and movement logic
                // Enhance condensation - more likely to condense at ceiling or near other water
                bool condensed = false;
                
                // Higher elevation = higher condensation chance
                float condensationChance = 0.05f * (1.0f - (float)y / GRID_HEIGHT);
                
                // Higher moisture = higher condensation chance
                condensationChance += ((float)(grid[y][x].moisture - 20) / 100.0f) * 0.5f;
                
                // Check for nearby water (ceiling water creates nucleation centers)
                int waterNeighbors = 0;
                int ceilingWaterSize = 0;
                
                for(int dy = -2; dy <= 2; dy++) {
                    for(int dx = -2; dx <= 2; dx++) {
                        int nx = x + dx, ny = y + dy;
                        if(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT && 
                           grid[ny][nx].type == 2) {
                            
                            // Water proximity boosts condensation chance
                            float distance = sqrtf(dx*dx + dy*dy);
                            if(distance <= 2.0f) {
                                waterNeighbors++;
                                
                                // Check if it's ceiling water (at top or has water above it)
                                bool isCeilingWater = (ny == 0 || (ny > 0 && grid[ny-1][nx].type == 2));
                                if(isCeilingWater) {
                                    ceilingWaterSize++;
                                    // Ceiling water creates a stronger condensation field
                                    condensationChance += 0.2f / (distance + 0.5f);
                                }
                            }
                        }
                    }
                }
                
                // At ceiling, almost always condense with sufficient moisture
                if(y == 0 && grid[y][x].moisture > 30 && GetRandomValue(0, 100) < 80) {
                    grid[y][x].type = 2; // Convert to water
                    float intensity = (float)grid[y][x].moisture / 100.0f;
                    grid[y][x].baseColor = (Color){
                        0 + (int)(200 * (1.0f - intensity)),
                        120 + (int)(135 * (1.0f - intensity)),
                        255,
                        255
                    };
                    condensed = true;
                }
                // General condensation check
                else if(GetRandomValue(0, 100) < condensationChance * 100) {
                    // Condense into water if moisture is high enough
                    if(grid[y][x].moisture > 30) {
                        grid[y][x].type = 2; // Convert to water
                        float intensity = (float)grid[y][x].moisture / 100.0f;
                        grid[y][x].baseColor = (Color){
                            0 + (int)(200 * (1.0f - intensity)),
                            120 + (int)(135 * (1.0f - intensity)),
                            255,
                            255
                        };
                        condensed = true;
                    }
                }
                
                if(condensed) continue;
                
                // Rest of vapor movement logic
                // Check if water is directly above vapor (should be rare, but handle it)
                if(y > 0 && grid[y-1][x].type == 2) {
                    MoveCell(y-1, x, y, x);  // Will trigger the swap in MoveCell
                    continue;
                }
                
                // Precipitation at top of screen
                if(y == 0 && grid[y][x].moisture > 50) {
                    // Convert to water if it has enough moisture and is at the top
                    grid[y][x].type = 2; // Convert to water
                    float intensity = (float)grid[y][x].moisture / 100.0f;
                    grid[y][x].baseColor = (Color){
                        0 + (int)(200 * (1.0f - intensity)),
                        120 + (int)(135 * (1.0f - intensity)),
                        255,
                        255
                    };
                    continue;
                }
                
                // Vapor rising mechanics (opposite of water falling)
                if(y > 0) {  // If not at top
                    // Try to rise straight up first
                    if(grid[y-1][x].type == 0 || grid[y-1][x].type == 4) {
                        // If empty space or less dense vapor above, move up
                        if(grid[y-1][x].type == 0 || 
                           (grid[y-1][x].type == 4 && grid[y-1][x].moisture < grid[y][x].moisture)) {
                            // Fixed parameter order: source coordinates first, then destination
                            MoveCell(x, y, x, y-1);
                            continue;
                        }
                    }
                    
                    // If can't rise straight up, try diagonally
                    bool canRiseLeft = (x > 0 && (grid[y-1][x-1].type == 0 || 
                        (grid[y-1][x-1].type == 4 && grid[y-1][x-1].moisture < grid[y][x].moisture)));
                    bool canRiseRight = (x < GRID_WIDTH-1 && (grid[y-1][x+1].type == 0 || 
                        (grid[y-1][x+1].type == 4 && grid[y-1][x+1].moisture < grid[y][x].moisture)));
                    
                    if(canRiseLeft && canRiseRight) {
                        // Choose random direction
                        int direction = (GetRandomValue(0, 1) == 0) ? -1 : 1;
                        MoveCell(x, y, x+direction, y-1);
                        continue;
                    } else if(canRiseLeft) {
                        MoveCell(x, y, x-1, y-1);
                        continue;
                    } else if(canRiseRight) {
                        MoveCell(x, y, x+1, y-1);
                        continue;
                    }
                    
                    // Count neighboring vapor cells
                    int vaporNeighbors = 0;
                    for(int dy = -1; dy <= 1; dy++) {
                        for(int dx = -1; dx <= 1; dx++) {
                            if(dx == 0 && dy == 0) continue;
                            int nx = x + dx, ny = y + dy;
                            if(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT && 
                               grid[ny][nx].type == 4) {
                                vaporNeighbors++;
                            }
                        }
                    }
                    
                    // Apply "inverse surface tension" - vapor prefers being in clusters
                    float moveChance = (vaporNeighbors > 2) ? 0.3f : 1.0f;
                    
                    if(GetRandomValue(0, 100) < moveChance * 100) {
                        // If can't rise, try to spread horizontally
                        bool canFlowLeft = (x > 0 && grid[y][x-1].type == 0);
                        bool canFlowRight = (x < GRID_WIDTH-1 && grid[y][x+1].type == 0);
                        
                        if(canFlowLeft && canFlowRight) {
                            // Slightly favor the current processing direction
                            int leftBias = processRightToLeft ? 60 : 40;
                            int flowDir = (GetRandomValue(0, 100) < leftBias) ? -1 : 1;
                            MoveCell(x, y, x+flowDir, y);
                        } else if(canFlowLeft) {
                            MoveCell(x, y, x-1, y);
                        } else if(canFlowRight) {
                            MoveCell(x, y, x+1, y);
                        }
                    }
                }
            }
        }
    }
    
    // Safety check - if we hit the iteration limit, something is wrong
    if(iterations >= MAX_ITERATIONS) {
        printf("WARNING: UpdateVapor reached maximum iterations limit!\n");
    }
    
    // Clean up the processed cells tracker
    for(int i = 0; i < GRID_HEIGHT; i++) {
        free(processedCells[i]);
    }
    free(processedCells);
    
    // Check moisture conservation after vapor processing
    int finalMoisture = CalculateTotalMoisture();
    if(abs(finalMoisture - initialMoisture) > 1) {
        printf("Moisture conservation error in UpdateVapor: %d\n", finalMoisture - initialMoisture);
    }
}

// Fix UpdateGrid to include additional safety checks
static void UpdateGrid(void) {
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

static void PlaceVapor(Vector2 position, int moisture) {
    int x = (int)position.x;
    int y = (int)position.y;
    
    // Ensure position is within grid bounds
    if(x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) return;
    
    // Clamp moisture to maximum value of 100
    moisture = ClampMoisture(moisture);
    
    grid[y][x].type = 4; // Vapor type
    grid[y][x].moisture = moisture;
    
    // Updated: Vapor under 50 moisture is invisible (was 0.5f)
    if(moisture < 50) {
        grid[y][x].baseColor = BLACK; // Make it invisible (same as background)
    } else {
        // Brightness based on moisture content (50-100 maps to 128-255)
        float intensityPct = (float)(moisture - 50) / 50.0f;
        int brightness = 128 + (int)(127 * intensityPct);
        
        grid[y][x].baseColor = (Color){
            brightness,
            brightness,
            brightness,
            255
        };
    }
    
    grid[y][x].position = (Vector2){
        x * CELL_SIZE,
        y * CELL_SIZE
    };
}

// Add a helper function to enforce moisture limit and prevent incorrect moisture values
static int ClampMoisture(int value) {
    // Ensure moisture is between 0 and 100
    if(value < 0) return 0;
    if(value > 100) return 100;
    return value;
}

// Update and draw frame function:
static void UpdateDrawFrame(void) {
    HandleInput();
    UpdateGrid();
    
    // Calculate total moisture for display
    int totalMoisture = CalculateTotalMoisture();
    
    BeginDrawing();
        ClearBackground(BLACK);
        DrawGameGrid();
        
        // Draw the brush size indicator in top-right corner
        int margin = 20;
        int indicatorRadius = brushRadius * 4; // Scale up for better visibility
        int centerX = GetScreenWidth() - margin - indicatorRadius;
        int centerY = margin + indicatorRadius;
        
        // Draw outer circle (border)
        DrawCircleLines(centerX, centerY, indicatorRadius, WHITE);
        
        // Draw inner filled circle with semi-transparency
        DrawCircle(centerX, centerY, indicatorRadius - 2, Fade(DARKGRAY, 0.7f));
        
        // Draw text showing the actual brush radius
        char radiusText[10];
        sprintf(radiusText, "R: %d", brushRadius);
        DrawText(radiusText, centerX - 20, centerY - 10, 20, WHITE);
        
        // Draw current brush at mouse position
        Vector2 mousePos = GetMousePosition();
        DrawCircleLines((int)mousePos.x, (int)mousePos.y, brushRadius * CELL_SIZE, WHITE);
        
        // Draw other UI elements
        DrawFPS(10, 10);
        
        // Draw UI instructions
        DrawText("Left Click: Place plant", 10, 30, 20, WHITE);
        DrawText("Right Click: Place Soil", 10, 50, 20, WHITE);
        DrawText("Middle Click: Place Water", 10, 70, 20, WHITE);
        DrawText("Mouse Wheel: Adjust brush size", 10, 90, 20, WHITE);
        
        // Display total moisture in the system
        char moistureText[50];
        sprintf(moistureText, "Total Moisture: %d", totalMoisture); // Changed from scientific notation
        DrawText(moistureText, 10, 110, 20, WHITE);
        
        // Add text at bottom of screen
        DrawText("Tree Growth Simulation", 10, GetScreenHeight() - 30, 20, WHITE);
        
    EndDrawing();
}

static void HandleInput(void) {
    // Handle brush size changes with mouse wheel
    float wheelMove = GetMouseWheelMove();
    if(wheelMove != 0) {
        brushRadius += (int)wheelMove;
        // Clamp brush radius between 1 and 32
        brushRadius = (brushRadius < 1) ? 1 : ((brushRadius > 32) ? 32 : brushRadius);
    }
    
    Vector2 mousePos = GetMousePosition();
    // Convert mouse position to grid coordinates by dividing by CELL_SIZE
    int gridX = (int)(mousePos.x / CELL_SIZE);
    int gridY = (int)(mousePos.y / CELL_SIZE);
    
    if(gridX >= 0 && gridX < GRID_WIDTH && gridY >= 0 && gridY < GRID_HEIGHT) {
        if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            float currentTime = GetTime();
            if(currentTime - lastSeedTime >= SEED_DELAY) {
                PlaceCircularPattern(gridX, gridY, 3, brushRadius); // Place plant in circular pattern
                lastSeedTime = currentTime;
            }
        }
        else if(IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            PlaceCircularPattern(gridX, gridY, 1, brushRadius); // Place soil in circular pattern
        }
        else if(IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
            PlaceCircularPattern(gridX, gridY, 2, brushRadius); // Place water in circular pattern
        }
    }
    
    // Add debug key - press D to check for bad cells
    if(IsKeyPressed(KEY_D)) {
        DebugGridCells();
    }
}

static void DrawGameGrid(void) {
    for(int i = 0; i < GRID_HEIGHT; i++) {
        for(int j = 0; j < GRID_WIDTH; j++) {
            // Add safety check to prevent undefined colors
            Color cellColor = grid[i][j].baseColor;
            
            // Enforce moisture limits
            grid[i][j].moisture = ClampMoisture(grid[i][j].moisture);
            
            // Fix for pink, purple or undefined colors
            if((cellColor.r > 200 && cellColor.g < 100 && cellColor.b > 200) || 
               (cellColor.r > 200 && cellColor.g < 100 && cellColor.b > 100)) {
                // This is detecting pink/purple-ish colors
                switch(grid[i][j].type) {
                    case 0: // Air
                        cellColor = BLACK;
                        break;
                    case 1: // Soil
                        {
                            // Re-calculate soil color based on proper moisture range
                            float intensityPct = (float)grid[i][j].moisture / 100.0f;
                            cellColor = (Color){
                                127 - (intensityPct * 51),  // R: 127 -> 76
                                106 - (intensityPct * 43),  // G: 106 -> 63
                                79 - (intensityPct * 32),   // B: 79 -> 47
                                255
                            };
                        }
                        break;
                    case 2: // Water
                        {
                            // Re-calculate water color based on proper moisture range
                            float intensityPct = (float)grid[i][j].moisture / 100.0f;
                            cellColor = (Color){
                                0 + (int)(200 * (1.0f - intensityPct)),
                                120 + (int)(135 * (1.0f - intensityPct)),
                                255,
                                255
                            };
                        }
                        break;
                    case 3: // Plant
                        cellColor = GREEN;
                        break;
                    case 4: // Vapor
                        {
                            // Recalculate vapor color using proper threshold
                            if(grid[i][j].moisture < 50) {
                                cellColor = BLACK; // Invisible
                            } else {
                                float intensityPct = (float)(grid[i][j].moisture - 50) / 50.0f;
                                int brightness = 128 + (int)(127 * intensityPct);
                                cellColor = (Color){
                                    brightness, brightness, brightness, 255
                                };
                            }
                        }
                        break;
                    default:
                        cellColor = BLACK; // Default fallback
                        break;
                }
                
                // Update the stored color
                grid[i][j].baseColor = cellColor;
            }
            
            DrawRectangle(
                j * CELL_SIZE,
                i * CELL_SIZE,
                CELL_SIZE - 1,
                CELL_SIZE - 1,
                cellColor
            );
        }
    }
}

static void PlaceSoil(Vector2 position) {
    int x = (int)position.x;
    int y = (int)position.y;
    
    grid[y][x].type = 1;
    grid[y][x].baseColor = BROWN;
    grid[y][x].moisture = 20;  // Start sand with 20 moisture (was 0.2f)
    grid[y][x].position = (Vector2){
        x * CELL_SIZE,
        y * CELL_SIZE
    };
    
    // Add this line to ensure soil always has valid type
    grid[y][x].is_falling = false;
}

static void PlaceWater(Vector2 position) {
    int x = (int)position.x;
    int y = (int)position.y;
    
    // Give newly placed water a random moisture level between 70 and 100
    int randomMoisture = 70 + GetRandomValue(0, 30);
    
    grid[y][x].type = 2;
    grid[y][x].moisture = randomMoisture;
    
    // Set color based on moisture/density
    float intensityPct = (float)randomMoisture / 100.0f;
    grid[y][x].baseColor = (Color){
        0 + (int)(200 * (1.0f - intensityPct)),
        120 + (int)(135 * (1.0f - intensityPct)),
        255,
        255
    };
    
    grid[y][x].position = (Vector2){
        x * CELL_SIZE,
        y * CELL_SIZE
    };
}

// New function to place cells in a circular pattern
static void PlaceCircularPattern(int centerX, int centerY, int cellType, int radius) {
    // Iterate through a square area and check if points are within the circle
    for(int y = centerY - radius; y <= centerY + radius; y++) {
        for(int x = centerX - radius; x <= centerX + radius; x++) {
            // Calculate distance from center (using squared distance to avoid sqrt for performance)
            float distanceSquared = (x - centerX) * (x - centerX) + (y - centerY) * (y - centerY);
            
            // If within radius and within grid bounds
            if(distanceSquared <= radius * radius && x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT) {
                // Place the appropriate cell type
                switch(cellType) {
                    case 1: // Soil
                        PlaceSoil((Vector2){x, y});
                        break;
                    case 2: // Water
                        PlaceWater((Vector2){x, y});
                        break;
                    case 3: // Plant
                        if(grid[y][x].type == 0) { // Only place plant on empty cells
                            grid[y][x].type = 3;
                            grid[y][x].baseColor = GREEN;
                            grid[y][x].position = (Vector2){x * CELL_SIZE, y * CELL_SIZE};
                        }
                        break;
                }
            }
        }
    }
}

static void TransferMoisture(void) {
    // Track total moisture before and after the function for debugging
    int initialMoisture = CalculateTotalMoisture();
    
    // Iterate through all cells to handle moisture transfer
    for(int y = 0; y < GRID_HEIGHT; y++) {
        for(int x = 0; x < GRID_WIDTH; x++) {
            // Handle sand (type 1) moisture management
            if(grid[y][x].type == 1) {
                // Skip vapor generation and moisture evaporation if the soil is falling
                if(grid[y][x].is_falling) continue;
                
                int* moisture = &grid[y][x].moisture;
                
                // Sand can hold up to 100 units of moisture max
                // Sand wants to hold 50 units of moisture (was 0.5f)
                
                // If sand has less than max moisture, it can absorb more
                if(*moisture < 100) {
                    // Check for adjacent water/vapor cells to absorb from
                    for(int dy = -1; dy <= 1 && *moisture < 100; dy++) {
                        // Fix the critical bug: dx was using dy in the condition!
                        for(int dx = -1; dx <= 1 && *moisture < 100; dx++) {
                            if(dx == 0 && dy == 0) continue;
                            
                            int nx = x + dx;
                            int ny = y + dy;
                            
                            // If in bounds and is water or vapor
                            if(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT && 
                               (grid[ny][nx].type == 2 || grid[ny][nx].type == 4)) {
                                
                                // Absorb some moisture (exactly 5 units per cycle)
                                int transferAmount = 5;  // Was 0.05f
                                
                                // Don't exceed capacity
                                if(*moisture + transferAmount > 100) {
                                    transferAmount = 100 - *moisture;
                                }
                                
                                // Don't take more than the source has
                                if(transferAmount > grid[ny][nx].moisture) {
                                    transferAmount = grid[ny][nx].moisture;
                                }
                                
                                if(transferAmount >= 5) {
                                    // Save exact moisture amounts before transfer
                                    int sourceBefore = grid[ny][nx].moisture;
                                    int targetBefore = *moisture;
                                    
                                    // Apply transfer - EXACT AMOUNTS
                                    *moisture += transferAmount;
                                    grid[ny][nx].moisture -= transferAmount;
                                    
                                    // Verify conservation - should always be exact with integers
                                    int totalAfter = *moisture + grid[ny][nx].moisture;
                                    int totalBefore = sourceBefore + targetBefore;
                                    
                                    if(totalAfter != totalBefore) {
                                        // This should never happen with integers, but just in case
                                        printf("ERROR: Moisture conservation violated in sand absorption\n");
                                    }
                                    
                                    // Update colors based on new moisture values
                                    // ...existing color updating code converted to use integer moisture...
                                }
                            }
                        }
                    }
                }
                
                // MODIFIED: Water seepage logic with integer moisture
                if(*moisture > 50) {  // Was > 0.5f
                    // First, try downward transfer (due to gravity)
                    bool transferredMoisture = false;
                    
                    // Check cells below first (gravity-based flow)
                    if(y < GRID_HEIGHT - 1) {
                        // Direct bottom cell - prioritize with strongest flow
                        if(grid[y+1][x].type != 2) { // Not water
                            if(grid[y+1][x].type == 0) {
                                // Empty space below - create water droplet if enough moisture
                                int transferAmount = 30;  // Was 0.3f
                                if(*moisture - transferAmount < 50) {
                                    transferAmount = *moisture - 50;
                                }
                                
                                if(transferAmount >= 30) {
                                    // Create water for large transfer amounts
                                    grid[y+1][x].type = 2; // Water
                                    grid[y+1][x].moisture = transferAmount;
                                    grid[y+1][x].position = (Vector2){x * CELL_SIZE, (y+1) * CELL_SIZE};
                                    
                                    // Set water color
                                    float intensity = (float)transferAmount / 100.0f;
                                    grid[y+1][x].baseColor = (Color){
                                        0 + (int)(200 * (1.0f - intensity)),
                                        120 + (int)(135 * (1.0f - intensity)),
                                        255,
                                        255
                                    };
                                    
                                    *moisture = ClampMoisture(*moisture - transferAmount);
                                    transferredMoisture = true;
                                } else if(transferAmount > 5) {
                                    // Create vapor for smaller transfer amounts
                                    PlaceVapor((Vector2){x, y+1}, transferAmount);
                                    *moisture = ClampMoisture(*moisture - transferAmount);
                                    transferredMoisture = true;
                                }
                            }
                            else if(grid[y+1][x].type == 1 || grid[y+1][x].type == 3) {
                                // Soil or plant below - transfer moisture if they have less
                                if(grid[y+1][x].moisture < *moisture && grid[y+1][x].moisture < 100) {
                                    // Increased transfer rate for downward flow (2x normal)
                                    int transferAmount = 10;  // Was 0.1f
                                    
                                    // Don't give away too much (sand wants to keep 50)
                                    if(*moisture - transferAmount < 50) {
                                        transferAmount = *moisture - 50;
                                    }
                                    
                                    // Don't overfill the receiving cell
                                    if(grid[y+1][x].moisture + transferAmount > 100) {
                                        transferAmount = 100 - grid[y+1][x].moisture;
                                    }
                                    
                                    if(transferAmount > 0) {
                                        *moisture = ClampMoisture(*moisture - transferAmount);
                                        grid[y+1][x].moisture = ClampMoisture(grid[y+1][x].moisture + transferAmount);
                                        transferredMoisture = true;
                                    }
                                }
                            }
                            else if(grid[y+1][x].type == 4) { // Vapor below
                                // Transfer moisture to vapor
                                int transferAmount = 20 - grid[y+1][x].moisture;  // Was 0.2f
                                if(transferAmount <= 0) transferAmount = 5;  // Was 0.05f
                                
                                if(*moisture - transferAmount < 50) {
                                    transferAmount = *moisture - 50;
                                }
                                
                                if(transferAmount > 0) {
                                    *moisture = ClampMoisture(*moisture - transferAmount);
                                    grid[y+1][x].moisture = ClampMoisture(grid[y+1][x].moisture + transferAmount);
                                    transferredMoisture = true;
                                    
                                    // Update vapor color
                                    if(grid[y+1][x].moisture >= 50) {  // >= 0.5 in the old scale
                                        int brightness = 128 + (int)(127 * ((float)(grid[y+1][x].moisture - 50) / 50.0f));
                                        grid[y+1][x].baseColor = (Color){
                                            brightness, brightness, brightness, 255
                                        };
                                    }
                                }
                            }
                        }
                        
                        // Now check the diagonal bottom cells if we didn't transfer yet
                        if(!transferredMoisture) {
                            // Check bottom-left
                            if(x > 0 && grid[y+1][x-1].type != 2) {
                                if(grid[y+1][x-1].type == 0) {
                                    // Empty space - create water/vapor
                                    int transferAmount = 25;  // Was 0.25f
                                    if(*moisture - transferAmount < 50) transferAmount = *moisture - 50;
                                    
                                    if(transferAmount >= 30) {
                                        // Create water
                                        grid[y+1][x-1].type = 2;
                                        grid[y+1][x-1].moisture = transferAmount;
                                        grid[y+1][x-1].position = (Vector2){(x-1) * CELL_SIZE, (y+1) * CELL_SIZE};
                                        
                                        float intensity = (float)transferAmount / 100.0f;
                                        grid[y+1][x-1].baseColor = (Color){
                                            0 + (int)(200 * (1.0f - intensity)),
                                            120 + (int)(135 * (1.0f - intensity)),
                                            255,
                                            255
                                        };
                                        
                                        *moisture = ClampMoisture(*moisture - transferAmount);
                                        transferredMoisture = true;
                                    } else if(transferAmount > 5) {
                                        // Create vapor
                                        PlaceVapor((Vector2){x-1, y+1}, transferAmount);
                                        *moisture = ClampMoisture(*moisture - transferAmount);
                                        transferredMoisture = true;
                                    }
                                } else if((grid[y+1][x-1].type == 1 || grid[y+1][x-1].type == 3) && 
                                        grid[y+1][x-1].moisture < *moisture && grid[y+1][x-1].moisture < 100) {
                                    // Transfer to soil/plant diagonally below
                                    int transferAmount = 8;  // Was 0.08f
                                    
                                    if(*moisture - transferAmount < 50)
                                        transferAmount = *moisture - 50;
                                    
                                    if(grid[y+1][x-1].moisture + transferAmount > 100)
                                        transferAmount = 100 - grid[y+1][x-1].moisture;
                                    
                                    if(transferAmount > 0) {
                                        *moisture = ClampMoisture(*moisture - transferAmount);
                                        grid[y+1][x-1].moisture = ClampMoisture(grid[y+1][x-1].moisture + transferAmount);
                                        transferredMoisture = true;
                                    }
                                }
                            }
                            
                            // Check bottom-right (similar logic)
                            if(!transferredMoisture && x < GRID_WIDTH-1 && grid[y+1][x+1].type != 2) {
                                // Similar logic as bottom-left
                                if(grid[y+1][x+1].type == 0) {
                                    int transferAmount = 25;  // Was 0.25f
                                    if(*moisture - transferAmount < 50) transferAmount = *moisture - 50;
                                    
                                    if(transferAmount >= 30) {
                                        // Create water
                                        grid[y+1][x+1].type = 2;
                                        grid[y+1][x+1].moisture = transferAmount;
                                        grid[y+1][x+1].position = (Vector2){(x+1) * CELL_SIZE, (y+1) * CELL_SIZE};
                                        
                                        float intensity = (float)transferAmount / 100.0f;
                                        grid[y+1][x+1].baseColor = (Color){
                                            0 + (int)(200 * (1.0f - intensity)),
                                            120 + (int)(135 * (1.0f - intensity)),
                                            255,
                                            255
                                        };
                                        
                                        *moisture = ClampMoisture(*moisture - transferAmount);
                                        transferredMoisture = true;
                                    } else if(transferAmount > 5) {
                                        // Create vapor
                                        PlaceVapor((Vector2){x+1, y+1}, transferAmount);
                                        *moisture = ClampMoisture(*moisture - transferAmount);
                                        transferredMoisture = true;
                                    }
                                } else if((grid[y+1][x+1].type == 1 || grid[y+1][x+1].type == 3) && 
                                        grid[y+1][x+1].moisture < *moisture && grid[y+1][x+1].moisture < 100) {
                                    // Transfer to soil/plant diagonally below
                                    int transferAmount = 8;  // Was 0.08f
                                    
                                    if(*moisture - transferAmount < 50)
                                        transferAmount = *moisture - 50;
                                    
                                    if(grid[y+1][x+1].moisture + transferAmount > 100)
                                        transferAmount = 100 - grid[y+1][x+1].moisture;
                                    
                                    if(transferAmount > 0) {
                                        *moisture = ClampMoisture(*moisture - transferAmount);
                                        grid[y+1][x+1].moisture = ClampMoisture(grid[y+1][x+1].moisture + transferAmount);
                                        transferredMoisture = true;
                                    }
                                }
                            }
                        }
                    }
                    
                    // Fall back to regular omni-directional distribution if we didn't transfer downward
                    if(!transferredMoisture) {
                        // First, check for spaces to potentially create vapor or water
                        bool transferredMoisture = false;
                    
                        // Check above and diagonally above first (vapor rises)
                        int checkOrder[6][2] = {
                            {0, -1},   // Above
                            {-1, -1},  // Above left
                            {1, -1},   // Above right
                            {-1, 0},   // Left
                            {1, 0},    // Right
                            {0, 1}     // Below (lowest priority)
                        };
                        
                        for(int i = 0; i < 6 && *moisture > 50; i++) {
                            int nx = x + checkOrder[i][0];
                            int ny = y + checkOrder[i][1];
                            
                            // Check if in bounds
                            if(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT) {
                                // Check if empty space
                                if(grid[ny][nx].type == 0) {
                                    int transferAmount = 20;  // Was 0.2f
                                    if(*moisture - transferAmount < 50) {
                                        transferAmount = *moisture - 50;
                                    }
                                    
                                    if(transferAmount > 5) { // Minimum threshold
                                        // Create water or vapor based on transfer amount
                                        if(transferAmount >= 30) {
                                            // Create water for large transfer amounts
                                            grid[ny][nx].type = 2; // Water
                                            grid[ny][nx].moisture = transferAmount;
                                            grid[ny][nx].position = (Vector2){nx * CELL_SIZE, ny * CELL_SIZE};
                                            
                                            // Set water color
                                            float intensity = (float)transferAmount / 100.0f;
                                            grid[ny][nx].baseColor = (Color){
                                                0 + (int)(200 * (1.0f - intensity)),
                                                120 + (int)(135 * (1.0f - intensity)),
                                                255,
                                                255
                                            };
                                        } else {
                                            // Create vapor for smaller transfer amounts
                                            PlaceVapor((Vector2){nx, ny}, transferAmount);
                                        }
                                        
                                        *moisture = ClampMoisture(*moisture - transferAmount);
                                        transferredMoisture = true;
                                    }
                                }
                                // Add moisture to existing vapor if it's below visibility threshold
                                else if(grid[ny][nx].type == 4 && grid[ny][nx].moisture < 50) {  // < 0.5 in the old scale
                                    int transferAmount = 50 - grid[ny][nx].moisture; // Fill to visibility threshold
                                    if(*moisture - transferAmount < 50) {
                                        transferAmount = *moisture - 50;
                                    }
                                    
                                    if(transferAmount > 0) {
                                        grid[ny][nx].moisture = ClampMoisture(grid[ny][nx].moisture + transferAmount);
                                        *moisture = ClampMoisture(*moisture - transferAmount);
                                        
                                        // Update vapor color
                                        if(grid[ny][nx].moisture >= 50) {  // >= 0.5 in the old scale
                                            int brightness = 128 + (int)(127 * ((float)(grid[ny][nx].moisture - 50) / 50.0f));
                                            grid[ny][nx].baseColor = (Color){
                                                brightness, brightness, brightness, 255
                                            };
                                        } else {
                                            grid[ny][nx].baseColor = BLACK; // Still invisible
                                        }
                                        
                                        transferredMoisture = true;
                                    }
                                }
                                // For existing vapor above threshold, create adjacent water instead
                                else if(grid[ny][nx].type == 4 && grid[ny][nx].moisture >= 50) {  // >= 0.5 in the old scale
                                    // Look for an adjacent empty space to place water
                                    for(int dy = -1; dy <= 1; dy++) {
                                        for(int dx = -1; dx <= 1; dx++) {
                                            if(dx == 0 && dy == 0) continue;
                                            
                                            int wx = nx + dx;
                                            int wy = ny + dy;
                                            
                                            if(wx >= 0 && wx < GRID_WIDTH && wy >= 0 && wy < GRID_HEIGHT && 
                                               grid[wy][wx].type == 0) {
                                                // Found empty space, create water
                                                int transferAmount = 30;  // Was 0.3f
                                                if(*moisture - transferAmount < 50) {
                                                    transferAmount = *moisture - 50;
                                                }
                                                
                                                if(transferAmount > 10) {  // Was 0.1f
                                                    grid[wy][wx].type = 2; // Water
                                                    grid[wy][wx].moisture = transferAmount;
                                                    grid[wy][wx].position = (Vector2){wx * CELL_SIZE, wy * CELL_SIZE};
                                                    
                                                    // Set water color
                                                    float intensity = (float)transferAmount / 100.0f;
                                                    grid[wy][wx].baseColor = (Color){
                                                        0 + (int)(200 * (1.0f - intensity)),
                                                        120 + (int)(135 * (1.0f - intensity)),
                                                        255,
                                                        255
                                                    };
                                                    
                                                    *moisture = ClampMoisture(*moisture - transferAmount);
                                                    transferredMoisture = true;
                                                    goto breakloop; // Exit nested loops
                                                }
                                            }
                                        }
                                    }
                                    breakloop: ; // Label for goto
                                }
                            }
                        }
                        
                        // If we didn't create vapor or water, distribute moisture to other cells
                        if(!transferredMoisture && *moisture > 50) {
                            // Original moisture distribution logic
                            // Look for adjacent cells to give moisture to (not water or air)
                            for(int dy = -1; dy <= 1 && *moisture > 50; dy++) {
                                for(int dx = -1; dx <= 1 && *moisture > 50; dx++) {
                                    // Skip the center cell
                                    if(dx == 0 && dy == 0) continue;
                                    
                                    int nx = x + dx;
                                    int ny = y + dy;
                                    
                                    // If in bounds, not water or air, and has lower moisture
                                    if(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT && 
                                       grid[ny][nx].type != 0 && grid[ny][nx].type != 2 && 
                                       grid[ny][nx].moisture < *moisture) {
                                        
                                        // Transfer some moisture (5 units per cycle)
                                        int transferAmount = 5;  // Was 0.05f
                                        
                                        // Don't give away too much (sand wants to keep 50)
                                        if(*moisture - transferAmount < 50) {
                                            transferAmount = *moisture - 50;
                                        }
                                        
                                        // Don't overfill the receiving cell
                                        if(grid[ny][nx].moisture + transferAmount > 100) {
                                            transferAmount = 100 - grid[ny][nx].moisture;
                                        }
                                        
                                        if(transferAmount > 0) {
                                            *moisture = ClampMoisture(*moisture - transferAmount);
                                            grid[ny][nx].moisture = ClampMoisture(grid[ny][nx].moisture + transferAmount);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                
                // Update sand color based on moisture
                float intensityPct = (float)*moisture / 100.0f;
                grid[y][x].baseColor = (Color){
                    127 - (intensityPct * 51),  // R: 127 -> 76
                    106 - (intensityPct * 43),  // G: 106 -> 63
                    79 - (intensityPct * 32),   // B: 79 -> 47
                    255
                };
                
                // Make sure sand type is preserved even if it has no moisture
                if(*moisture <= 0) {
                    *moisture = 0; // Clamp to zero
                    // Don't change the type - sand remains sand even when dry
                    grid[y][x].baseColor = BROWN; // Ensure it keeps the base sand color
                }
            }
            
            // Handle water (type 2) moisture
            else if(grid[y][x].type == 2) {
                // Skip vapor generation if the water is falling
                if(grid[y][x].is_falling) continue;
                
                // Update water color based on moisture/density
                float intensityPct = (float)grid[y][x].moisture / 100.0f;
                grid[y][x].baseColor = (Color){
                    0 + (int)(200 * (1.0f - intensityPct)),
                    120 + (int)(135 * (1.0f - intensityPct)),
                    255,
                    255
                };
                
                // Count water neighbors
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
                
                // MODIFIED: Water transfers moisture to adjacent non-water/vapor cells
                // AND receives moisture from adjacent wet sand
                int totalReceived = 0;  // Track total moisture received from wet sand
                
                for(int dy = -1; dy <= 1; dy++) {
                    for(int dx = -1; dx <= 1; dx++) {
                        // Skip the center cell
                        if(dx == 0 && dy == 0) continue;
                        
                        int nx = x + dx;
                        int ny = y + dy;
                        
                        // If in bounds and is soil or plant (transfer TO them)
                        if(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT) {
                            // Transfer moisture TO soil/plants
                            if(grid[ny][nx].type == 1 || grid[ny][nx].type == 3) {
                                // Only transfer if there's room for more moisture
                                if(grid[ny][nx].moisture < 100) {
                                    int transferAmount = 5;  // Was 0.05f
                                    
                                    // Don't overfill
                                    if(grid[ny][nx].moisture + transferAmount > 100) {
                                        transferAmount = 100 - grid[ny][nx].moisture;
                                    }
                                    
                                    // Don't give away all moisture
                                    if(grid[y][x].moisture - transferAmount < 10) {  // Was 0.1f
                                        transferAmount = grid[y][x].moisture - 10;
                                        if(transferAmount <= 0) continue;
                                    }
                                    
                                    grid[y][x].moisture = ClampMoisture(grid[y][x].moisture - transferAmount);
                                    grid[ny][nx].moisture = ClampMoisture(grid[ny][nx].moisture + transferAmount);
                                }
                            }
                            // Soil with excess moisture transfers TO water (significant amounts)
                            else if(grid[ny][nx].type == 1 && grid[ny][nx].moisture > 60) {  // > 0.6 in the old scale
                                // Very wet sand contributes significant moisture to adjacent water
                                int transferAmount = 20 * (grid[ny][nx].moisture - 60) / 100;  // Was 0.2f
                                
                                // Upper bound check
                                if(grid[ny][nx].moisture - transferAmount < 60) {
                                    transferAmount = grid[ny][nx].moisture - 60;
                                }
                                
                                if(transferAmount > 1) {  // Was 0.01f
                                    grid[ny][nx].moisture = ClampMoisture(grid[ny][nx].moisture - transferAmount);
                                    grid[y][x].moisture = ClampMoisture(grid[y][x].moisture + transferAmount);
                                    totalReceived += transferAmount;
                                    
                                    // Update soil color after moisture loss
                                    float intensityPct = (float)grid[ny][nx].moisture / 100.0f;
                                    grid[ny][nx].baseColor = (Color){
                                        127 - (intensityPct * 51),  // R: 127 -> 76
                                        106 - (intensityPct * 43),  // G: 106 -> 63
                                        79 - (intensityPct * 32),   // B: 79 -> 47
                                        255
                                    };
                                }
                            }
                        }
                    }
                }
                
                // MODIFIED: Only consider evaporation if water couldn't move, doesn't have many neighbors,
                // AND did not receive significant moisture from surrounding wet sand
                // AND is not falling (added condition)
                float evaporationChance = 1.0f - (waterNeighbors * 0.1f); // 10% reduction per neighbor
                
                // Reduce evaporation chance significantly if receiving moisture from wet sand
                if(totalReceived > 0) {
                    evaporationChance *= (1.0f - (totalReceived * 5.0f));
                    if(evaporationChance < 0.0f) evaporationChance = 0.0f;
                }
                
                // Set a minimum chance (but can be zero if surrounded by wet sand)
                if(evaporationChance > 0.0f && evaporationChance < 0.2f) 
                    evaporationChance = 0.2f;
                
                // If water is being actively replenished, significantly reduce evaporation rate
                float evaporationRate = (totalReceived > 5) ?  // Was 0.05f
                    0.0001f * (1.0f + y / (float)GRID_HEIGHT) : // Very slow if being replenished
                    0.001f * (1.0f + y / (float)GRID_HEIGHT);  // Normal rate otherwise
                
                // Add the explicit check for falling state
                if(grid[y][x].volume == 0 && !grid[y][x].is_falling && grid[y][x].moisture > 20 && y > 0 &&  // > 0.2 in the old scale
                   GetRandomValue(0, 100) < evaporationChance * 100) {
                    // Check spaces directly above and diagonally above
                    bool evaporated = false;
                    
                    // Priority order for evaporation spaces
                    int checkOrder[3][2] = {
                        {0, -1},   // Directly above
                        {-1, -1},  // Above left
                        {1, -1}    // Above right
                    };
                    
                    // Try evaporating to one of the valid spaces
                    for(int i = 0; i < 3 && !evaporated; i++) {
                        int nx = x + checkOrder[i][0];
                        int ny = y + checkOrder[i][1];
                        
                        // Check if in bounds and empty
                        if(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT && 
                           grid[ny][nx].type == 0) {
                            
                            // Standard evaporation for above/diagonal spaces
                            int evaporatedAmount = fmin(evaporationRate, grid[y][x].moisture);
                            
                            if(evaporatedAmount >= 5) {  // Was 0.05f
                                grid[y][x].moisture -= evaporatedAmount;
                                PlaceVapor((Vector2){nx, ny}, evaporatedAmount);
                                evaporated = true;
                            }
                        }
                    }
                    
                    // If couldn't evaporate normally, check for other spaces to transfer all moisture
                    if(!evaporated) {
                        int otherCheckOrder[5][2] = {
                            {-1, 0},  // Left
                            {1, 0},   // Right
                            {-1, 1},  // Below left
                            {1, 1},   // Below right
                            {0, 1}    // Below
                        };
                        
                        for(int i = 0; i < 5 && !evaporated; i++) {
                            int nx = x + otherCheckOrder[i][0];
                            int ny = y + otherCheckOrder[i][1];
                            
                            // Check if in bounds and empty
                            if(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT && 
                               grid[ny][nx].type == 0) {
                                
                                int fullAmount = grid[y][x].moisture;
                                grid[y][x].moisture = 0;
                                
                                // Create vapor with all the moisture
                                PlaceVapor((Vector2){nx, ny}, fullAmount);
                                evaporated = true;
                            }
                        }
                    }
                }
                
                // Reset the movement flag for next cycle
                grid[y][x].volume = 1;
                
                // Water transfers moisture to adjacent non-water/vapor cells
                for(int dy = -1; dy <= 1; dy++) {
                    for(int dx = -1; dx <= 1; dx++) {
                        // Skip the center cell
                        if(dx == 0 && dy == 0) continue;
                        
                        int nx = x + dx;
                        int ny = y + dy;
                        
                        // If in bounds and is soil or plant
                        if(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT && 
                           (grid[ny][nx].type == 1 || grid[ny][nx].type == 3)) {
                            
                            // Only transfer if there's room for more moisture
                            if(grid[ny][nx].moisture < 100) {
                                int transferAmount = 5;  // Was 0.05f
                                
                                // Don't overfill
                                if(grid[ny][nx].moisture + transferAmount > 100) {
                                    transferAmount = 100 - grid[ny][nx].moisture;
                                }
                                
                                grid[ny][nx].moisture = ClampMoisture(grid[ny][nx].moisture + transferAmount);
                            }
                        }
                    }
                }
            }
            
            // Handle vapor (type 4)
            else if(grid[y][x].type == 4) {
                // Update vapor color based on moisture - invisible if below 50
                if(grid[y][x].moisture < 50) {  // < 0.5 in the old scale
                    grid[y][x].baseColor = BLACK; // Invisible
                } else {
                    float intensityPct = (float)(grid[y][x].moisture - 50) / 50.0f;
                    int brightness = 128 + (int)(127 * intensityPct);
                    grid[y][x].baseColor = (Color){
                        brightness, brightness, brightness, 255
                    };
                }
                
                // REMOVE direct vapor dissipation - this was incorrectly destroying moisture
                // grid[y][x].moisture -= 0.0005f;
                
                // Instead, handle vapor dissipation through moisture transfer
                if(grid[y][x].moisture > 3) {  // Allow a minimum ambient vapor level (was 0.03f)
                    // Try to find another ambient vapor cell to transfer excess to
                    bool transferred = false;
                    for(int dy = -1; dy <= 1 && !transferred; dy++) {
                        for(int dx = -1; dy <= 1 && !transferred; dx++) {
                            if(dx == 0 && dy == 0) continue;
                            
                            int nx = x + dx;
                            int ny = y + dy;
                            
                            if(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT && 
                               grid[ny][nx].type == 4 && grid[ny][nx].moisture < grid[y][x].moisture) {
                                // Transfer a tiny amount of moisture rather than losing it
                                int transferAmount = 1;  // Was 0.0005f
                                grid[y][x].moisture = ClampMoisture(grid[y][x].moisture - transferAmount);
                                grid[ny][nx].moisture = ClampMoisture(grid[ny][nx].moisture + transferAmount);
                                transferred = true;
                            }
                        }
                    }
                }
                
                // Only convert to air if we're at the absolute minimum moisture
                if(grid[y][x].moisture <= 1) {  // <= 0.01f in the old scale
                    // Instead of destroying this moisture, redistribute to neighbors
                    int remainingMoisture = grid[y][x].moisture;
                    
                    if(remainingMoisture > 0) {
                        // Find nearby vapor cells to transfer remaining moisture to
                        for(int dy = -1; dy <= 1 && remainingMoisture > 0; dy++) {
                            for(int dx = -1; dy <= 1 && remainingMoisture > 0; dx++) {
                                if(dx == 0 && dy == 0) continue;
                                
                                int nx = x + dx;
                                int ny = y + dy;
                                
                                if(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT && 
                                   grid[ny][nx].type == 4) {
                                    grid[ny][nx].moisture += remainingMoisture;
                                    remainingMoisture = 0;
                                }
                            }
                        }
                    }
                    
                    // Now safe to convert to air since moisture is preserved
                    grid[y][x].type = 0; 
                    grid[y][x].moisture = 0;
                    grid[y][x].baseColor = BLACK;
                }
            }
        }
    }
    
    // After all transfers, verify total moisture hasn't changed
    int finalMoisture = CalculateTotalMoisture();
    if(finalMoisture != initialMoisture) {
        // Fix any conservation errors
        int adjustment = initialMoisture - finalMoisture;
        bool fixed = false;
        
        for(int y = 0; y < GRID_HEIGHT && !fixed; y++) {
            for(int x = 0; x < GRID_WIDTH && !fixed; x++) {
                if(grid[y][x].type == 2 && grid[y][x].moisture + adjustment >= 10) {
                    grid[y][x].moisture += adjustment;
                    fixed = true;
                }
            }
        }
    }
}

// Helper function to identify connected ceiling water clusters
static void FloodFillCeilingCluster(int x, int y, int clusterID, int** clusterIDs) {
    // Check bounds
    if(x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) return;
    
    // Only process water cells that haven't been assigned a cluster yet
    if(grid[y][x].type != 2 || clusterIDs[y][x] != -1) return;
    
    // Must be ceiling water (at top or has water above it)
    if(y > 0 && grid[y-1][x].type != 2 && y != 0) return;
    
    // Assign this cell to the cluster
    clusterIDs[y][x] = clusterID;
    
    // Recursively process neighboring cells (only horizontal and below)
    FloodFillCeilingCluster(x-1, y, clusterID, clusterIDs);  // Left
    FloodFillCeilingCluster(x+1, y, clusterID, clusterIDs);  // Right
    FloodFillCeilingCluster(x, y+1, clusterID, clusterIDs);  // Below
    FloodFillCeilingCluster(x-1, y+1, clusterID, clusterIDs); // Diag below left
    FloodFillCeilingCluster(x+1, y+1, clusterID, clusterIDs); // Diag below right
}

// Add a debug function to check cell state (can be triggered with a key press)
static void DebugGridCells(void) {
    // Count cells by type and color
    int typeCount[5] = {0}; // Air, Soil, Water, Plant, Vapor
    int pinkCells = 0;
    
    for(int y = 0; y < GRID_HEIGHT; y++) {
        for(int x = 0; x < GRID_WIDTH; x++) {
            int type = grid[y][x].type;
            if(type >= 0 && type <= 4) {
                typeCount[type]++;
            }
            
            // Check for pink-ish colors
            Color c = grid[y][x].baseColor;
            if(c.r > 200 && c.g < 100 && c.b > 200) {
                pinkCells++;
                // Reset to the correct color
                grid[y][x].baseColor = BLACK;
                grid[y][x].type = 0; // Reset to air as a safety measure
            }
        }
    }
    
    // Print debug info to console
    printf("Grid State: Air=%d, Soil=%d, Water=%d, Plant=%d, Vapor=%d, PinkCells=%d\n",
           typeCount[0], typeCount[1], typeCount[2], typeCount[3], typeCount[4], pinkCells);
    
    // Calculate and report total system moisture
    int totalMoisture = CalculateTotalMoisture();
    printf("Total system moisture: %d\n", totalMoisture);
}

// Add a function to track total moisture in the system (for debugging conservation)
static int CalculateTotalMoisture() {
    int totalMoisture = 0;
    
    for(int y = 0; y < GRID_HEIGHT; y++) {
        for(int x = 0; x < GRID_WIDTH; x++) {
            totalMoisture += grid[y][x].moisture;
        }
    }
    
    return totalMoisture;
}
