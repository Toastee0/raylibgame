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
    float moisture; //moisture level of the pixel, 0-8 each point of moisture can move to an adjacent cell by diffusion. but only if the cell is permeable. 
    int permeable; //0 = impermeable, 1 = permeable (water permeable)
    int age; //age of the object, used for plant growth and reproduction.
    int maxage; //max age of the object, used for plant growth and reproduction.
    int temperature; //temperature of the object.
    int freezingpoint; //freezing point of the object.
    int boilingpoint; //boiling point of the object.
    int temperaturepreferanceoffset; 
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
static void CopyCell(int x, int y, int x2, int y2);
static void PlaceCircularPattern(int centerX, int centerY, int cellType, int radius);
static void TransferMoisture(void);
static void UpdateVapor(void);
static void PlaceVapor(Vector2 position, float moisture);
static void FloodFillCeilingCluster(int x, int y, int clusterID, int** clusterIDs);
static void DebugGridCells(void); // Add debug function


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

// Initialize the grid
static void InitGrid(void) {
    grid = (GridCell**)malloc(GRID_HEIGHT * sizeof(GridCell*));
    for(int i = 0; i < GRID_HEIGHT; i++) {
        grid[i] = (GridCell*)malloc(GRID_WIDTH * sizeof(GridCell));
        for(int j = 0; j < GRID_WIDTH; j++) {
            grid[i][j].position = (Vector2){j * CELL_SIZE, i * CELL_SIZE};
            grid[i][j].baseColor = BLACK;
            
            // Initialize all empty cells as very low moisture vapor
            grid[i][j].type = 4;  // Vapor type
            grid[i][j].moisture = 0.02f;  // Minimal moisture
            grid[i][j].permeable = 1;
        }
    }
}

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
        // Swap the cells - sand falls through water
        GridCell temp = grid[y2][x2];
        grid[y2][x2] = grid[y][x];
        grid[y][x] = temp;
        
        // Update positions for both cells
        grid[y2][x2].position = (Vector2){x2 * CELL_SIZE, y2 * CELL_SIZE};
        grid[y][x].position = (Vector2){x * CELL_SIZE, y * CELL_SIZE};
        
        return;
    }
    
    // Special handling for water falling through vapor
    if(grid[y][x].type == 2 && grid[y2][x2].type == 4) {
        // Swap the cells - water falls through vapor
        GridCell temp = grid[y2][x2];
        grid[y2][x2] = grid[y][x];
        grid[y][x] = temp;
        
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
    
    // Update the position property of the moved cell
    grid[y2][x2].position = (Vector2){x2 * CELL_SIZE, y2 * CELL_SIZE};
    
    // Reset the source cell
    grid[y][x].type = 0;
    grid[y][x].baseColor = BLACK;
    grid[y][x].moisture = 0.0f;
}

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
                // Track soil moisture
                float* moisture = &grid[y][x].moisture;

                // FALLING MECHANICS
                // First check directly below
                if(y < GRID_HEIGHT - 1) {  // If not at bottom of grid
                    // Fall through empty space or water
                    if(grid[y+1][x].type == 0 || grid[y+1][x].type == 2) {
                        // Move soil down one cell (will swap with water if needed)
                        MoveCell(x, y, x, y+1);
                        continue;  // Skip to next iteration after moving
                    }
                    
                    // If can't fall straight down, try diagonal slides
                    bool canSlideLeft = (x > 0 && (grid[y+1][x-1].type == 0 || grid[y+1][x-1].type == 2));
                    bool canSlideRight = (x < GRID_WIDTH-1 && (grid[y+1][x+1].type == 0 || grid[y+1][x+1].type == 2));
                    
                    if(canSlideLeft && canSlideRight) {
                        // Choose random direction, but slightly favor the current processing direction
                        // This helps prevent directional bias
                        int bias = processRightToLeft ? 40 : 60; // 40/60 bias instead of 50/50
                        int slideDir = (GetRandomValue(0, 100) < bias) ? -1 : 1;
                        MoveCell(x, y, x+slideDir, y+1);
                    } else if(canSlideLeft) {
                        MoveCell(x, y, x-1, y+1);
                    } else if(canSlideRight) {
                        MoveCell(x, y, x+1, y+1);
                    }
                }

                // Update soil color based on moisture
                if(*moisture > 0) {
                    // Interpolate between BROWN and DARKBROWN based on moisture
                    grid[y][x].baseColor = (Color){
                        127 - (*moisture * 51),  // R: 127 -> 76
                        106 - (*moisture * 43),  // G: 106 -> 63
                        79 - (*moisture * 32),   // B: 79 -> 47
                        255
                    };
                }

                // REMOVE moisture evaporation - this was incorrectly destroying moisture
                // *moisture = fmax(*moisture - 0.001f, 0.0f);
                
                // Instead, transfer tiny amounts to ambient vapor if needed
                if(grid[y][x].moisture > 0.2f) {
                    // Try to find nearby ambient vapor to add moisture to
                    bool transferred = false;
                    for(int dy = -1; dy <= 1 && !transferred; dy++) {
                        for(int dx = -1; dx <= 1 && !transferred; dx++) {
                            if(dx == 0 && dy == 0) continue;
                            
                            int nx = x + dx;
                            int ny = y + dy;
                            
                            if(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT && 
                               grid[ny][nx].type == 4) {
                                // Transfer a tiny amount of moisture
                                float transferAmount = 0.001f;
                                if(grid[y][x].moisture - transferAmount < 0.2f) {
                                    transferAmount = grid[y][x].moisture - 0.2f;
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
                
                // Store whether this water cell has moved in this update cycle
                // This will be used in TransferMoisture to determine if evaporation should be considered
                if(!hasMoved && grid[y][x].type == 2) {  // Double-check it's still water
                    grid[y][x].volume = 0;  // Use volume field to flag that water didn't move
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
                    float avgMoisture = (grid[y][x].moisture + grid[y][x-1].moisture) / 2.0f;
                    // Small equalization step (don't fully equalize)
                    grid[y][x].moisture = grid[y][x].moisture * 0.9f + avgMoisture * 0.1f;
                    grid[y][x-1].moisture = grid[y][x-1].moisture * 0.9f + avgMoisture * 0.1f;
                }
                if(x < GRID_WIDTH-1 && grid[y][x+1].type == 2) {
                    // Average densities for adjacent water cells
                    float avgMoisture = (grid[y][x].moisture + grid[y][x+1].moisture) / 2.0f;
                    // Small equalization step
                    grid[y][x].moisture = grid[y][x].moisture * 0.9f + avgMoisture * 0.1f;
                    grid[y][x+1].moisture = grid[y][x+1].moisture * 0.9f + avgMoisture * 0.1f;
                }
            }
        }
    }
}

static void UpdateVapor(void) {
    // Randomly decide initial direction for this cycle (for horizontal movement)
    bool processRightToLeft = GetRandomValue(0, 1);
    
    // Process vapor from top to bottom (opposite of water), alternating left/right direction
    for(int y = 0; y < GRID_HEIGHT; y++) {
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
            if(grid[y][x].type == 4) {  // If it's vapor
                // Enhance condensation - more likely to condense at ceiling or near other water
                bool condensed = false;
                
                // Higher elevation = higher condensation chance
                float condensationChance = 0.05f * (1.0f - (float)y / GRID_HEIGHT);
                
                // Higher moisture = higher condensation chance
                condensationChance += (grid[y][x].moisture - 0.2f) * 0.5f;
                
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
                if(y == 0 && grid[y][x].moisture > 0.3f && GetRandomValue(0, 100) < 80) {
                    grid[y][x].type = 2; // Convert to water
                    grid[y][x].baseColor = (Color){
                        0 + (int)(200 * (1.0f - grid[y][x].moisture)),
                        120 + (int)(135 * (1.0f - grid[y][x].moisture)),
                        255,
                        255
                    };
                    condensed = true;
                }
                // General condensation check
                else if(GetRandomValue(0, 100) < condensationChance * 100) {
                    // Condense into water if moisture is high enough
                    if(grid[y][x].moisture > 0.3f) {
                        grid[y][x].type = 2; // Convert to water
                        grid[y][x].baseColor = (Color){
                            0 + (int)(200 * (1.0f - grid[y][x].moisture)),
                            120 + (int)(135 * (1.0f - grid[y][x].moisture)),
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
                if(y == 0 && grid[y][x].moisture > 0.5f) {
                    // Convert to water if it has enough moisture and is at the top
                    grid[y][x].type = 2; // Convert to water
                    grid[y][x].baseColor = (Color){
                        0 + (int)(200 * (1.0f - grid[y][x].moisture)),
                        120 + (int)(135 * (1.0f - grid[y][x].moisture)),
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
    
    // After basic vapor movement, handle vertical moisture transfers (inverted density from water)
    for(int y = GRID_HEIGHT-1; y >= 0; y--) {  // Process from bottom to top
        for(int x = 0; x < GRID_WIDTH; x++) {
            if(grid[y][x].type == 4) {  // If it's vapor
                // Check for vapor cells above that could receive moisture
                if(y > 0 && grid[y-1][x].type == 4) {
                    // If this vapor is more dense (has more moisture) than vapor above, transfer up
                    if(grid[y][x].moisture > grid[y-1][x].moisture && grid[y-1][x].moisture < 0.8f) {
                        // Calculate transfer amount - smaller than water transfers
                        float transferAmount = (grid[y][x].moisture - grid[y-1][x].moisture) * 0.05f;
                        
                        // Transfer moisture upward
                        grid[y-1][x].moisture += transferAmount;
                        grid[y][x].moisture -= transferAmount;
                        
                        // Update colors
                        if(grid[y-1][x].moisture >= 0.2f) {
                            int brightness = 128 + (int)(127 * grid[y-1][x].moisture);
                            grid[y-1][x].baseColor = (Color){
                                brightness, brightness, brightness, 255
                            };
                        }
                        
                        if(grid[y][x].moisture < 0.2f) {
                            grid[y][x].baseColor = BLACK; // Invisible
                        } else {
                            int brightness = 128 + (int)(127 * grid[y][x].moisture);
                            grid[y][x].baseColor = (Color){
                                brightness, brightness, brightness, 255
                            };
                        }
                        
                        continue;
                    }
                }
                
                // Also check diagonally upward
                bool checkedDiagonals = false;
                if(y > 0) {
                    // Check left-up diagonal
                    if(x > 0 && grid[y-1][x-1].type == 4) {
                        if(grid[y][x].moisture > grid[y-1][x-1].moisture && grid[y-1][x-1].moisture < 0.8f) {
                            float transferAmount = (grid[y][x].moisture - grid[y-1][x-1].moisture) * 0.03f;
                            grid[y-1][x-1].moisture += transferAmount;
                            grid[y][x].moisture -= transferAmount;
                            
                            // Update colors
                            if(grid[y-1][x-1].moisture >= 0.2f) {
                                int brightness = 128 + (int)(127 * grid[y-1][x-1].moisture);
                                grid[y-1][x-1].baseColor = (Color){
                                    brightness, brightness, brightness, 255
                                };
                            }
                            
                            checkedDiagonals = true;
                        }
                    }
                    
                    // Check right-up diagonal
                    if(!checkedDiagonals && x < GRID_WIDTH-1 && grid[y-1][x+1].type == 4) {
                        if(grid[y][x].moisture > grid[y-1][x+1].moisture && grid[y-1][x+1].moisture < 0.8f) {
                            float transferAmount = (grid[y][x].moisture - grid[y-1][x+1].moisture) * 0.03f;
                            grid[y-1][x+1].moisture += transferAmount;
                            grid[y][x].moisture -= transferAmount;
                            
                            // Update colors
                            if(grid[y-1][x+1].moisture >= 0.2f) {
                                int brightness = 128 + (int)(127 * grid[y-1][x+1].moisture);
                                grid[y-1][x+1].baseColor = (Color){
                                    brightness, brightness, brightness, 255
                                };
                            }
                        }
                    }
                }
                
                // Update vapor color after all transfers
                if(grid[y][x].moisture < 0.2f) {
                    grid[y][x].baseColor = BLACK; // Invisible
                } else {
                    int brightness = 128 + (int)(127 * grid[y][x].moisture);
                    grid[y][x].baseColor = (Color){
                        brightness, brightness, brightness, 255
                    };
                }
            }
        }
    }
}

static void PlaceVapor(Vector2 position, float moisture) {
    int x = (int)position.x;
    int y = (int)position.y;
    
    // Ensure position is within grid bounds
    if(x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) return;
    
    // Clamp moisture to maximum value of 1.0
    if(moisture > 1.0f) moisture = 1.0f;
    
    grid[y][x].type = 4; // Vapor type
    grid[y][x].moisture = moisture;
    
    // Updated: Vapor under 0.5 moisture is invisible
    if(moisture < 0.5f) {
        grid[y][x].baseColor = BLACK; // Make it invisible (same as background)
    } else {
        // Brightness based on moisture content for visible vapor (0.5 - 1.0 maps to 128-255)
        int brightness = 128 + (int)(127 * (moisture - 0.5f) * 2.0f); // Adjusted scaling
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
static float ClampMoisture(float value) {
    // Ensure moisture is between 0.0 and 1.0
    if(value < 0.0f) return 0.0f;
    if(value > 1.0f) return 1.0f;
    return value;
}

// Update and draw frame function:
static void UpdateDrawFrame(void) {
    HandleInput();
    UpdateGrid();
    
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
            if(grid[i][j].moisture > 1.0f) {
                grid[i][j].moisture = 1.0f;
            } else if(grid[i][j].moisture < 0.0f) {
                grid[i][j].moisture = 0.0f;
            }
            
            // Fix for pink, purple or undefined colors
            if((cellColor.r > 200 && cellColor.g < 100 && cellColor.b > 200) || 
               (cellColor.r > 200 && cellColor.g < 100 && cellColor.b > 100)) {
                // This is detecting pink/purple-ish colors
                switch(grid[i][j].type) {
                    case 0: // Air
                        cellColor = BLACK;
                        break;
                    case 1: // Soil
                        // Re-calculate soil color based on proper moisture range
                        grid[i][j].moisture = ClampMoisture(grid[i][j].moisture);
                        cellColor = (Color){
                            127 - (grid[i][j].moisture * 51),  // R: 127 -> 76
                            106 - (grid[i][j].moisture * 43),  // G: 106 -> 63
                            79 - (grid[i][j].moisture * 32),   // B: 79 -> 47
                            255
                        };
                        break;
                    case 2: // Water
                        // Re-calculate water color based on proper moisture range
                        grid[i][j].moisture = ClampMoisture(grid[i][j].moisture);
                        cellColor = (Color){
                            0 + (int)(200 * (1.0f - grid[i][j].moisture)),
                            120 + (int)(135 * (1.0f - grid[i][j].moisture)),
                            255,
                            255
                        };
                        break;
                    case 3: // Plant
                        cellColor = GREEN;
                        break;
                    case 4: // Vapor
                        // Recalculate vapor color using proper threshold
                        grid[i][j].moisture = ClampMoisture(grid[i][j].moisture);
                        if(grid[i][j].moisture < 0.5f) {
                            cellColor = BLACK; // Invisible
                        } else {
                            int brightness = 128 + (int)(127 * (grid[i][j].moisture - 0.5f) * 2.0f);
                            cellColor = (Color){
                                brightness, brightness, brightness, 255
                            };
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

static void UpdateGrid(void) {
    UpdateWater();
    UpdateSoil();
    UpdateVapor(); // Add vapor update step
    
    // Handle moisture transfer at the end of each cycle
    TransferMoisture();
}

static void PlaceSoil(Vector2 position) {
    grid[(int)position.y][(int)position.x].type = 1;
    grid[(int)position.y][(int)position.x].baseColor = BROWN;
    grid[(int)position.y][(int)position.x].moisture = 0.2f;  // Start sand with 0.2 moisture
    grid[(int)position.y][(int)position.x].position = (Vector2){
        position.x * CELL_SIZE,
        position.y * CELL_SIZE
    };
}

static void PlaceWater(Vector2 position) {
    int x = (int)position.x;
    int y = (int)position.y;
    
    // Give newly placed water a random moisture level between 0.7 and 1.0
    // This creates variations in water density for more interesting behavior
    float randomMoisture = 0.7f + (float)GetRandomValue(0, 30) / 100.0f;
    
    grid[y][x].type = 2;
    grid[y][x].moisture = randomMoisture;
    
    // Set color based on moisture/density
    float intensity = randomMoisture;
    grid[y][x].baseColor = (Color){
        0 + (int)(200 * (1.0f - intensity)),
        120 + (int)(135 * (1.0f - intensity)),
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
    // Iterate through all cells to handle moisture transfer
    for(int y = 0; y < GRID_HEIGHT; y++) {
        for(int x = 0; x < GRID_WIDTH; x++) {
            // Handle sand (type 1) moisture management
            if(grid[y][x].type == 1) {
                float* moisture = &grid[y][x].moisture;
                
                // Sand can hold up to 1.0 units of moisture max
                // Sand wants to hold 0.5 units of moisture
                
                // If sand has less than max moisture, it can absorb more
                if(*moisture < 1.0f) {
                    // Check for adjacent water/vapor cells to absorb from
                    for(int dy = -1; dy <= 1 && *moisture < 1.0f; dy++) {
                        // Fix the critical bug: dx was using dy in the condition!
                        for(int dx = -1; dx <= 1 && *moisture < 1.0f; dx++) {
                            if(dx == 0 && dy == 0) continue;
                            
                            int nx = x + dx;
                            int ny = y + dy;
                            
                            // If in bounds and is water or vapor
                            if(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT && 
                               (grid[ny][nx].type == 2 || grid[ny][nx].type == 4)) {
                                
                                // Absorb some moisture (0.05 per cycle)
                                float transferAmount = 0.05f;
                                if(*moisture + transferAmount > 1.0f) {
                                    transferAmount = 1.0f - *moisture;
                                }
                                
                                // Don't take more than the source has
                                if(transferAmount > grid[ny][nx].moisture) {
                                    transferAmount = grid[ny][nx].moisture;
                                }
                                
                                *moisture = ClampMoisture(*moisture + transferAmount);
                                grid[ny][nx].moisture = ClampMoisture(grid[ny][nx].moisture - transferAmount);
                                
                                // Update source cell color based on new moisture
                                if(grid[ny][nx].type == 2) { // Water
                                    float intensity = grid[ny][nx].moisture;
                                    grid[ny][nx].baseColor = (Color){
                                        0 + (int)(200 * (1.0f - intensity)),
                                        120 + (int)(135 * (1.0f - intensity)),
                                        255,
                                        255
                                    };
                                } else if(grid[ny][nx].type == 4) { // Vapor
                                    // Update vapor color based on new moisture - invisible if below 0.5
                                    if(grid[ny][nx].moisture < 0.5f) {
                                        grid[ny][nx].baseColor = BLACK; // Invisible
                                    } else {
                                        int brightness = 128 + (int)(127 * (grid[ny][nx].moisture - 0.5f) * 2.0f);
                                        grid[ny][nx].baseColor = (Color){
                                            brightness, brightness, brightness, 255
                                        };
                                    }
                                }
                                
                                // Remove cells with no moisture
                                if(grid[ny][nx].moisture <= 0.01f) {
                                    grid[ny][nx].type = 0; // Convert to air
                                    grid[ny][nx].moisture = 0.0f;
                                    grid[ny][nx].baseColor = BLACK;
                                }
                            }
                        }
                    }
                }
                
                // MODIFIED: Water seepage logic - prioritize downward movement
                if(*moisture > 0.5f) {
                    // First, try downward transfer (due to gravity)
                    bool transferredMoisture = false;
                    
                    // Check cells below first (gravity-based flow)
                    if(y < GRID_HEIGHT - 1) {
                        // Direct bottom cell - prioritize with strongest flow
                        if(grid[y+1][x].type != 2) { // Not water
                            if(grid[y+1][x].type == 0) {
                                // Empty space below - create water droplet if enough moisture
                                float transferAmount = 0.3f;
                                if(*moisture - transferAmount < 0.5f) {
                                    transferAmount = *moisture - 0.5f;
                                }
                                
                                if(transferAmount >= 0.3f) {
                                    // Create water for large transfer amounts
                                    grid[y+1][x].type = 2; // Water
                                    grid[y+1][x].moisture = transferAmount;
                                    grid[y+1][x].position = (Vector2){x * CELL_SIZE, (y+1) * CELL_SIZE};
                                    
                                    // Set water color
                                    float intensity = transferAmount;
                                    grid[y+1][x].baseColor = (Color){
                                        0 + (int)(200 * (1.0f - intensity)),
                                        120 + (int)(135 * (1.0f - intensity)),
                                        255,
                                        255
                                    };
                                    
                                    *moisture = ClampMoisture(*moisture - transferAmount);
                                    transferredMoisture = true;
                                } else if(transferAmount > 0.05f) {
                                    // Create vapor for smaller transfer amounts
                                    PlaceVapor((Vector2){x, y+1}, transferAmount);
                                    *moisture = ClampMoisture(*moisture - transferAmount);
                                    transferredMoisture = true;
                                }
                            }
                            else if(grid[y+1][x].type == 1 || grid[y+1][x].type == 3) {
                                // Soil or plant below - transfer moisture if they have less
                                if(grid[y+1][x].moisture < *moisture && grid[y+1][x].moisture < 1.0f) {
                                    // Increased transfer rate for downward flow (2x normal)
                                    float transferAmount = 0.1f;
                                    
                                    // Don't give away too much (sand wants to keep 0.5)
                                    if(*moisture - transferAmount < 0.5f) {
                                        transferAmount = *moisture - 0.5f;
                                    }
                                    
                                    // Don't overfill the receiving cell
                                    if(grid[y+1][x].moisture + transferAmount > 1.0f) {
                                        transferAmount = 1.0f - grid[y+1][x].moisture;
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
                                float transferAmount = 0.2f - grid[y+1][x].moisture;
                                if(transferAmount <= 0) transferAmount = 0.05f;
                                
                                if(*moisture - transferAmount < 0.5f) {
                                    transferAmount = *moisture - 0.5f;
                                }
                                
                                if(transferAmount > 0) {
                                    *moisture = ClampMoisture(*moisture - transferAmount);
                                    grid[y+1][x].moisture = ClampMoisture(grid[y+1][x].moisture + transferAmount);
                                    transferredMoisture = true;
                                    
                                    // Update vapor color
                                    if(grid[y+1][x].moisture >= 0.5f) {
                                        int brightness = 128 + (int)(127 * (grid[y+1][x].moisture - 0.5f) * 2.0f);
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
                                    float transferAmount = 0.25f;
                                    if(*moisture - transferAmount < 0.5f) transferAmount = *moisture - 0.5f;
                                    
                                    if(transferAmount >= 0.3f) {
                                        // Create water
                                        grid[y+1][x-1].type = 2;
                                        grid[y+1][x-1].moisture = transferAmount;
                                        grid[y+1][x-1].position = (Vector2){(x-1) * CELL_SIZE, (y+1) * CELL_SIZE};
                                        
                                        float intensity = transferAmount;
                                        grid[y+1][x-1].baseColor = (Color){
                                            0 + (int)(200 * (1.0f - intensity)),
                                            120 + (int)(135 * (1.0f - intensity)),
                                            255,
                                            255
                                        };
                                        
                                        *moisture = ClampMoisture(*moisture - transferAmount);
                                        transferredMoisture = true;
                                    } else if(transferAmount > 0.05f) {
                                        // Create vapor
                                        PlaceVapor((Vector2){x-1, y+1}, transferAmount);
                                        *moisture = ClampMoisture(*moisture - transferAmount);
                                        transferredMoisture = true;
                                    }
                                } else if((grid[y+1][x-1].type == 1 || grid[y+1][x-1].type == 3) && 
                                        grid[y+1][x-1].moisture < *moisture && grid[y+1][x-1].moisture < 1.0f) {
                                    // Transfer to soil/plant diagonally below
                                    float transferAmount = 0.08f; // Slightly less than direct down
                                    
                                    if(*moisture - transferAmount < 0.5f)
                                        transferAmount = *moisture - 0.5f;
                                    
                                    if(grid[y+1][x-1].moisture + transferAmount > 1.0f)
                                        transferAmount = 1.0f - grid[y+1][x-1].moisture;
                                    
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
                                    float transferAmount = 0.25f;
                                    if(*moisture - transferAmount < 0.5f) transferAmount = *moisture - 0.5f;
                                    
                                    if(transferAmount >= 0.3f) {
                                        // Create water
                                        grid[y+1][x+1].type = 2;
                                        grid[y+1][x+1].moisture = transferAmount;
                                        grid[y+1][x+1].position = (Vector2){(x+1) * CELL_SIZE, (y+1) * CELL_SIZE};
                                        
                                        float intensity = transferAmount;
                                        grid[y+1][x+1].baseColor = (Color){
                                            0 + (int)(200 * (1.0f - intensity)),
                                            120 + (int)(135 * (1.0f - intensity)),
                                            255,
                                            255
                                        };
                                        
                                        *moisture = ClampMoisture(*moisture - transferAmount);
                                        transferredMoisture = true;
                                    } else if(transferAmount > 0.05f) {
                                        // Create vapor
                                        PlaceVapor((Vector2){x+1, y+1}, transferAmount);
                                        *moisture = ClampMoisture(*moisture - transferAmount);
                                        transferredMoisture = true;
                                    }
                                } else if((grid[y+1][x+1].type == 1 || grid[y+1][x+1].type == 3) && 
                                        grid[y+1][x+1].moisture < *moisture && grid[y+1][x+1].moisture < 1.0f) {
                                    // Transfer to soil/plant diagonally below
                                    float transferAmount = 0.08f; // Slightly less than direct down
                                    
                                    if(*moisture - transferAmount < 0.5f)
                                        transferAmount = *moisture - 0.5f;
                                    
                                    if(grid[y+1][x+1].moisture + transferAmount > 1.0f)
                                        transferAmount = 1.0f - grid[y+1][x+1].moisture;
                                    
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
                        
                        for(int i = 0; i < 6 && *moisture > 0.5f; i++) {
                            int nx = x + checkOrder[i][0];
                            int ny = y + checkOrder[i][1];
                            
                            // Check if in bounds
                            if(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT) {
                                // Check if empty space
                                if(grid[ny][nx].type == 0) {
                                    float transferAmount = 0.2f;
                                    if(*moisture - transferAmount < 0.5f) {
                                        transferAmount = *moisture - 0.5f;
                                    }
                                    
                                    if(transferAmount > 0.05f) { // Minimum threshold
                                        // Create water or vapor based on transfer amount
                                        if(transferAmount >= 0.3f) {
                                            // Create water for large transfer amounts
                                            grid[ny][nx].type = 2; // Water
                                            grid[ny][nx].moisture = transferAmount;
                                            grid[ny][nx].position = (Vector2){nx * CELL_SIZE, ny * CELL_SIZE};
                                            
                                            // Set water color
                                            float intensity = transferAmount;
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
                                else if(grid[ny][nx].type == 4 && grid[ny][nx].moisture < 0.5f) {
                                    float transferAmount = 0.5f - grid[ny][nx].moisture; // Fill to visibility threshold
                                    if(*moisture - transferAmount < 0.5f) {
                                        transferAmount = *moisture - 0.5f;
                                    }
                                    
                                    if(transferAmount > 0) {
                                        grid[ny][nx].moisture = ClampMoisture(grid[ny][nx].moisture + transferAmount);
                                        *moisture = ClampMoisture(*moisture - transferAmount);
                                        
                                        // Update vapor color
                                        if(grid[ny][nx].moisture >= 0.5f) {
                                            int brightness = 128 + (int)(127 * (grid[ny][nx].moisture - 0.5f) * 2.0f);
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
                                else if(grid[ny][nx].type == 4 && grid[ny][nx].moisture >= 0.5f) {
                                    // Look for an adjacent empty space to place water
                                    for(int dy = -1; dy <= 1; dy++) {
                                        for(int dx = -1; dx <= 1; dx++) {
                                            if(dx == 0 && dy == 0) continue;
                                            
                                            int wx = nx + dx;
                                            int wy = ny + dy;
                                            
                                            if(wx >= 0 && wx < GRID_WIDTH && wy >= 0 && wy < GRID_HEIGHT && 
                                               grid[wy][wx].type == 0) {
                                                // Found empty space, create water
                                                float transferAmount = 0.3f;
                                                if(*moisture - transferAmount < 0.5f) {
                                                    transferAmount = *moisture - 0.5f;
                                                }
                                                
                                                if(transferAmount > 0.1f) {
                                                    grid[wy][wx].type = 2; // Water
                                                    grid[wy][wx].moisture = transferAmount;
                                                    grid[wy][wx].position = (Vector2){wx * CELL_SIZE, wy * CELL_SIZE};
                                                    
                                                    // Set water color
                                                    float intensity = transferAmount;
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
                        if(!transferredMoisture && *moisture > 0.5f) {
                            // Original moisture distribution logic
                            // Look for adjacent cells to give moisture to (not water or air)
                            for(int dy = -1; dy <= 1 && *moisture > 0.5f; dy++) {
                                for(int dx = -1; dx <= 1 && *moisture > 0.5f; dx++) {
                                    // Skip the center cell
                                    if(dx == 0 && dy == 0) continue;
                                    
                                    int nx = x + dx;
                                    int ny = y + dy;
                                    
                                    // If in bounds, not water or air, and has lower moisture
                                    if(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT && 
                                       grid[ny][nx].type != 0 && grid[ny][nx].type != 2 && 
                                       grid[ny][nx].moisture < *moisture) {
                                        
                                        // Transfer some moisture (0.05 per cycle)
                                        float transferAmount = 0.05f;
                                        
                                        // Don't give away too much (sand wants to keep 0.5)
                                        if(*moisture - transferAmount < 0.5f) {
                                            transferAmount = *moisture - 0.5f;
                                        }
                                        
                                        // Don't overfill the receiving cell
                                        if(grid[ny][nx].moisture + transferAmount > 1.0f) {
                                            transferAmount = 1.0f - grid[ny][nx].moisture;
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
                if(*moisture > 0) {
                    // Interpolate between BROWN and DARKBROWN based on moisture
                    grid[y][x].baseColor = (Color){
                        127 - (*moisture * 51),  // R: 127 -> 76
                        106 - (*moisture * 43),  // G: 106 -> 63
                        79 - (*moisture * 32),   // B: 79 -> 47
                        255
                    };
                }
            }
            
            // Handle water (type 2) moisture
            else if(grid[y][x].type == 2) {
                // Update water color based on moisture/density
                float intensity = grid[y][x].moisture;
                grid[y][x].baseColor = (Color){
                    0 + (int)(200 * (1.0f - intensity)),
                    120 + (int)(135 * (1.0f - intensity)),
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
                float totalReceived = 0.0f;  // Track total moisture received from wet sand
                
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
                                if(grid[ny][nx].moisture < 1.0f) {
                                    float transferAmount = 0.05f;
                                    
                                    // Don't overfill
                                    if(grid[ny][nx].moisture + transferAmount > 1.0f) {
                                        transferAmount = 1.0f - grid[ny][nx].moisture;
                                    }
                                    
                                    // Don't give away all moisture
                                    if(grid[y][x].moisture - transferAmount < 0.1f) {
                                        transferAmount = grid[y][x].moisture - 0.1f;
                                        if(transferAmount <= 0) continue;
                                    }
                                    
                                    grid[y][x].moisture = ClampMoisture(grid[y][x].moisture - transferAmount);
                                    grid[ny][nx].moisture = ClampMoisture(grid[ny][nx].moisture + transferAmount);
                                }
                            }
                            // Soil with excess moisture transfers TO water (significant amounts)
                            else if(grid[ny][nx].type == 1 && grid[ny][nx].moisture > 0.6f) {
                                // Very wet sand contributes significant moisture to adjacent water
                                float transferAmount = 0.2f * (grid[ny][nx].moisture - 0.6f);
                                
                                // Upper bound check
                                if(grid[ny][nx].moisture - transferAmount < 0.6f) {
                                    transferAmount = grid[ny][nx].moisture - 0.6f;
                                }
                                
                                if(transferAmount > 0.01f) {
                                    grid[ny][nx].moisture = ClampMoisture(grid[ny][nx].moisture - transferAmount);
                                    grid[y][x].moisture = ClampMoisture(grid[y][x].moisture + transferAmount);
                                    totalReceived += transferAmount;
                                    
                                    // Update soil color after moisture loss
                                    grid[ny][nx].baseColor = (Color){
                                        127 - (grid[ny][nx].moisture * 51),  // R: 127 -> 76
                                        106 - (grid[ny][nx].moisture * 43),  // G: 106 -> 63
                                        79 - (grid[ny][nx].moisture * 32),   // B: 79 -> 47
                                        255
                                    };
                                }
                            }
                        }
                    }
                }
                
                // MODIFIED: Only consider evaporation if water couldn't move, doesn't have many neighbors,
                // AND did not receive significant moisture from surrounding wet sand
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
                float evaporationRate = (totalReceived > 0.05f) ? 
                    0.0001f * (1.0f + y / (float)GRID_HEIGHT) : // Very slow if being replenished
                    0.001f * (1.0f + y / (float)GRID_HEIGHT);  // Normal rate otherwise
                
                if(grid[y][x].volume == 0 && grid[y][x].moisture > 0.2f && y > 0 && 
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
                            float evaporatedAmount = fmin(evaporationRate, grid[y][x].moisture);
                            
                            if(evaporatedAmount > 0.0005f) {
                                grid[y][x].moisture = ClampMoisture(grid[y][x].moisture - evaporatedAmount);
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
                                
                                // Transfer ALL moisture to the space
                                float fullAmount = grid[y][x].moisture;
                                grid[y][x].moisture = 0.0f;
                                
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
                            if(grid[ny][nx].moisture < 1.0f) {
                                float transferAmount = 0.05f;
                                
                                // Don't overfill
                                if(grid[ny][nx].moisture + transferAmount > 1.0f) {
                                    transferAmount = 1.0f - grid[ny][nx].moisture;
                                }
                                
                                grid[ny][nx].moisture = ClampMoisture(grid[ny][nx].moisture + transferAmount);
                            }
                        }
                    }
                }
            }
            
            // Handle vapor (type 4)
            else if(grid[y][x].type == 4) {
                // Update vapor color based on moisture - invisible if below 0.5
                if(grid[y][x].moisture < 0.5f) {
                    grid[y][x].baseColor = BLACK; // Invisible
                } else {
                    int brightness = 128 + (int)(127 * (grid[y][x].moisture - 0.5f) * 2.0f);
                    grid[y][x].baseColor = (Color){
                        brightness, brightness, brightness, 255
                    };
                }
                
                // REMOVE direct vapor dissipation - this was incorrectly destroying moisture
                // grid[y][x].moisture -= 0.0005f;
                
                // Instead, handle vapor dissipation through moisture transfer
                if(grid[y][x].moisture > 0.03f) {  // Allow a minimum ambient vapor level
                    // Try to find another ambient vapor cell to transfer excess to
                    bool transferred = false;
                    for(int dy = -1; dy <= 1 && !transferred; dy++) {
                        for(int dx = -1; dx <= 1 && !transferred; dx++) {
                            if(dx == 0 && dy == 0) continue;
                            
                            int nx = x + dx;
                            int ny = y + dy;
                            
                            if(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT && 
                               grid[ny][nx].type == 4 && grid[ny][nx].moisture < grid[y][x].moisture) {
                                // Transfer a tiny amount of moisture rather than losing it
                                float transferAmount = 0.0005f;
                                grid[y][x].moisture = ClampMoisture(grid[y][x].moisture - transferAmount);
                                grid[ny][nx].moisture = ClampMoisture(grid[ny][nx].moisture + transferAmount);
                                transferred = true;
                            }
                        }
                    }
                }
                
                // Boost vapor buoyancy - increase moisture slightly to help vapor rise
                // Higher vapor should have more moisture unless at max
                if(grid[y][x].moisture > 0.03f && grid[y][x].moisture < 0.8f && GetRandomValue(0, 100) < 10) {
                    // Small random boost to help vapor rise (counteracting lateral diffusion)
                    grid[y][x].moisture += 0.001f;
                }
                
                // Only convert to air if we're at the absolute minimum moisture
                if(grid[y][x].moisture <= 0.01f) {
                    // Instead of destroying this moisture, redistribute to neighbors
                    float remainingMoisture = grid[y][x].moisture;
                    
                    if(remainingMoisture > 0) {
                        // Find nearby vapor cells to transfer remaining moisture to
                        for(int dy = -1; dy <= 1 && remainingMoisture > 0; dy++) {
                            for(int dx = -1; dx <= 1 && remainingMoisture > 0; dx++) {
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
                    grid[y][x].moisture = 0.0f;
                    grid[y][x].baseColor = BLACK;
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
}
