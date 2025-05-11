# Falling Sand Simulation Rules for C with Raylib

## Overview
This document outlines the rules and implementation guidelines for a falling sand-style simulation in C, using Raylib for rendering. The simulation operates on a 2D grid where each cell represents a primary material (air, water, soil, sand, mineral, sandstone, shale, limestone) and carries fractions of other materials. Designed for sequential CPU execution, it avoids CUDA-specific optimizations, using standard C libraries for randomness (`rand()`) and Raylib for visualization. The rules cover material behavior, density calculations, movement, erosion, deposition, compaction, evaporation, and user interaction, with instructions for integrating into existing C support code.

## Grid Representation
- **Structure**: Use a 2D grid stored as a 1D array for simplicity.
- **Dual Grids**: Maintain two grids (`current` and `next`) to prevent race conditions:
  - Read from `current`, write to `next`, then swap pointers after each update step.
  - Example allocation:
    ```c
    Cell *grid_current = (Cell*)malloc(sizeof(Cell) * GRID_WIDTH * GRID_HEIGHT);
    Cell *grid_next = (Cell*)malloc(sizeof(Cell) * GRID_WIDTH * GRID_HEIGHT);
    ```
- **Dimensions**: Suggested `GRID_WIDTH = 200`, `GRID_HEIGHT = 150` for manageable performance. Adjust based on your needs.
- **Swap**: After each simulation step:
    ```c
    Cell *temp = grid_current;
    grid_current = grid_next;
    grid_next = temp;
    ```

## Cell Structure
- Define a `Cell` struct to store material properties:
  ```c
  typedef struct {
      int material;          // Primary material (AIR, WATER, SOIL, etc.)
      float carried_fractions[4]; // Fractions of air, water, soil, mineral
      float density;         // Total density
  } Cell;
  ```
- **Material Enum**: Define materials as:
  ```c
  enum Material {
      AIR, WATER, SOIL, SAND, MINERAL, SANDSTONE, SHALE, LIMESTONE, MATERIAL_COUNT
  };
  ```

## Capacities
- Each material can carry fractions of other materials, limited by capacity:
  
  | Material      | Carries Air | Carries Water | Carries Soil | Carries Mineral |
  |---------------|-------------|---------------|--------------|-----------------|
  | **Air**       | 1.0         | 0.1           | 0.05         | 0.02           |
  | **Water**     | 0.0         | 1.0           | 0.1          | 0.1            |
  | **Soil**      | 0.1         | 0.1           | 1.0          | 0.05           |
  | **Sand**      | 0.1         | 0.1           | 0.0          | 0.05           |
  | **Mineral**   | 0.0         | 0.0           | 0.0          | 1.0            |
  | **Sandstone** | 0.05        | 0.05          | 0.0          | 0.02           |
  | **Shale**     | 0.02        | 0.02          | 0.0          | 0.01           |
  | **Limestone** | 0.01        | 0.01          | 0.0          | 0.01           |

- Store in a 2D array:
  ```c
  float capacities[MATERIAL_COUNT][4] = {
      {1.0f, 0.1f, 0.05f, 0.02f}, // Air
      {0.0f, 1.0f, 0.1f, 0.1f},   // Water
      {0.1f, 0.1f, 1.0f, 0.05f},  // Soil
      {0.1f, 0.1f, 0.0f, 0.05f},  // Sand
      {0.0f, 0.0f, 0.0f, 1.0f},   // Mineral
      {0.05f, 0.05f, 0.0f, 0.02f},// Sandstone
      {0.02f, 0.02f, 0.0f, 0.01f},// Shale
      {0.01f, 0.01f, 0.0f, 0.01f} // Limestone
  };
  ```

## Density
- **Base Densities**:
  - Air: 1.0
  - Water: 10.0
  - Soil: 20.0
  - Sand: 18.0
  - Mineral: 30.0
  - Sandstone: 25.0
  - Shale: 28.0
  - Limestone: 29.0
- **Total Density**:
  \[
  \text{density} = \text{base_density} + \sum (\text{carried_fraction} \times \text{carried_material_density})
  \]
- **Implementation**:
  ```c
  void compute_density(Cell *grid) {
      for (int y = 0; y < GRID_HEIGHT; y++) {
          for (int x = 0; x < GRID_WIDTH; x++) {
              int idx = y * GRID_WIDTH + x;
              float base_densities[] = {1.0f, 10.0f, 20.0f, 18.0f, 30.0f, 25.0f, 28.0f, 29.0f};
              grid[idx].density = base_densities[grid[idx].material];
              for (int i = 0; i < 4; i++) {
                  grid[idx].density += grid[idx].carried_fractions[i] * base_densities[i];
              }
          }
      }
  }
  ```

## Movement and Swapping
- **Air**:
  - Moves up or horizontally, swaps with air if denser than the cell above.
  - Implementation:
    ```c
    void air_density_swap(Cell *current, Cell *next) {
        for (int y = 1; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                int idx = y * GRID_WIDTH + x;
                int above_idx = (y - 1) * GRID_WIDTH + x;
                if (current[idx].material == AIR && current[above_idx].material == AIR &&
                    current[idx].density > current[above_idx].density) {
                    next[idx] = current[above_idx];
                    next[above_idx] = current[idx];
                } else {
                    next[idx] = current[idx];
                }
            }
        }
    }
    ```
- **Water**:
  - Moves down, swaps with air or denser water below.
  - Implementation:
    ```c
    void water_movement(Cell *current, Cell *next) {
        for (int y = 0; y < GRID_HEIGHT - 1; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                int idx = y * GRID_WIDTH + x;
                int below_idx = (y + 1) * GRID_WIDTH + x;
                if (current[idx].material == WATER && current[below_idx].material == AIR) {
                    next[idx] = current[below_idx];
                    next[below_idx] = current[idx];
                } else if (current[idx].material == WATER && current[below_idx].material == WATER &&
                           current[idx].density > current[below_idx].density) {
                    next[idx] = current[below_idx];
                    next[below_idx] = current[idx];
                } else {
                    next[idx] = current[idx];
                }
            }
        }
    }
    ```
- **Soil/Sand**:
  - Moves down, swaps with air or water (not mineral, sandstone, shale, limestone).
  - Implementation:
    ```c
    void soil_sand_movement(Cell *current, Cell *next) {
        for (int y = GRID_HEIGHT - 2; y >= 0; y--) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                int idx = y * GRID_WIDTH + x;
                int below_idx = (y + 1) * GRID_WIDTH + x;
                if ((current[idx].material == SOIL || current[idx].material == SAND) &&
                    (current[below_idx].material == AIR || current[below_idx].material == WATER)) {
                    next[idx] = current[below_idx];
                    next[below_idx] = current[idx];
                } else {
                    next[idx] = current[idx];
                }
            }
        }
    }
    ```
- **Sandstone**:
  - Moves down (50% chance, using `rand()`), swaps with air or water.
  - Implementation:
    ```c
    void sandstone_movement(Cell *current, Cell *next) {
        for (int y = GRID_HEIGHT - 2; y >= 0; y--) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                int idx = y * GRID_WIDTH + x;
                int below_idx = (y + 1) * GRID_WIDTH + x;
                if (current[idx].material == SANDSTONE &&
                    (current[below_idx].material == AIR || current[below_idx].material == WATER) &&
                    (rand() % 100) < 50) {
                    next[idx] = current[below_idx];
                    next[below_idx] = current[idx];
                } else {
                    next[idx] = current[idx];
                }
            }
        }
    }
    ```
- **Shale/Limestone/Mineral**:
  - Stationary, no movement.

## Water Pressure and Erosion
- **Pressure Calculation**:
  - Count water cells in a 5x3 region (5 horizontal, 3 vertical above, max 15).
  - Implementation:
    ```c
    float compute_pressure(Cell *current, int x, int y) {
        float pressure = 0.0f;
        if (current[y * GRID_WIDTH + x].material == WATER) {
            for (int dx = -2; dx <= 2; dx++) {
                for (int dy = 0; dy <= 2; dy++) {
                    int nx = x + dx;
                    int ny = y - dy;
                    if (nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT &&
                        current[ny * GRID_WIDTH + nx].material == WATER) {
                        pressure += 1.0f;
                    }
                }
            }
        }
        return pressure;
    }
    ```
- **Erosion**:
  - If pressure ≥ 4 (simplified resistance), transfer 0.02 soil from above soil cell to water, convert soil to air, 50% chance.
  - Implementation:
    ```c
    void erosion(Cell *current, Cell *next) {
        for (int y = 1; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                int idx = y * GRID_WIDTH + x;
                next[idx] = current[idx];
                if (current[idx].material == WATER && compute_pressure(current, x, y) >= 4.0f) {
                    int above_idx = (y - 1) * GRID_WIDTH + x;
                    if (y > 0 && current[above_idx].material == SOIL && (rand() % 100) < 50) {
                        next[idx].carried_fractions[SOIL] += 0.02f;
                        next[above_idx].material = AIR;
                        next[above_idx].carried_fractions[SOIL] = 0.0f;
                        next[above_idx].density = base_density[AIR];
                    }
                }
            }
        }
    }
    ```

## Deposition and Sedimentation
- **Water**:
  - If soil ≥ 0.01 and blocked below (not air), transfer 0.01 soil to below or create soil cell (0.5 soil).
  - If below is soil with ≥ 0.05 mineral, convert to sandstone (0.02 mineral, 0.05 water, 0.05 air).
  - Implementation:
    ```c
    void sedimentation(Cell *current, Cell *next) {
        for (int y = 0; y < GRID_HEIGHT - 1; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                int idx = y * GRID_WIDTH + x;
                int below_idx = (y + 1) * GRID_WIDTH + x;
                next[idx] = current[idx];
                if (current[idx].material == WATER && current[below_idx].material != AIR &&
                    current[idx].carried_fractions[SOIL] >= 0.01f) {
                    next[below_idx].carried_fractions[SOIL] += 0.01f;
                    next[idx].carried_fractions[SOIL] -= 0.01f;
                    if (current[below_idx].material == SOIL &&
                        next[below_idx].carried_fractions[MINERAL] >= 0.05f) {
                        next[below_idx].material = SANDSTONE;
                        next[below_idx].carried_fractions[MINERAL] = 0.02f;
                        next[below_idx].carried_fractions[WATER] = 0.05f;
                        next[below_idx].carried_fractions[AIR] = 0.05f;
                    }
                }
            }
        }
    }
    ```

## Compaction
- **Pressure**: Count solid cells (soil, sand, sandstone, shale, limestone, mineral) in a 3x3 grid above (max 9).
- **Rules**:
  - Soil/Sand: Pressure ≥ 3, 50% chance to sandstone.
  - Implementation:
    ```c
    void compaction(Cell *current, Cell *next) {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                int idx = y * GRID_WIDTH + x;
                next[idx] = current[idx];
                int pressure = 0;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        int nx = x + dx;
                        int ny = y + dy;
                        if (nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT &&
                            current[ny * GRID_WIDTH + nx].material != AIR &&
                            current[ny * GRID_WIDTH + nx].material != WATER) {
                            pressure++;
                        }
                    }
                }
                if (current[idx].material == SOIL && pressure >= 3 && (rand() % 100) < 50) {
                    next[idx].material = SANDSTONE;
                    next[idx].carried_fractions[MINERAL] = 0.02f;
                    next[idx].carried_fractions[WATER] = 0.05f;
                    next[idx].carried_fractions[AIR] = 0.05f;
                }
            }
        }
    }
    ```

## Evaporation
- Water transfers 0.01 water to adjacent air above (up to 0.1) per step.
- Implementation:
  ```c
  void evaporation(Cell *current, Cell *next) {
      for (int y = 1; y < GRID_HEIGHT; y++) {
          for (int x = 0; x < GRID_WIDTH; x++) {
              int idx = y * GRID_WIDTH + x;
              next[idx] = current[idx];
              if (current[idx].material == WATER && y > 0) {
                  int above_idx = (y - 1) * GRID_WIDTH + x;
                  if (current[above_idx].material == AIR &&
                      current[above_idx].carried_fractions[WATER] < 0.1f) {
                      next[above_idx].carried_fractions[WATER] += 0.01f;
                  }
              }
          }
      }
  }
  ```

## Visualization with Raylib
- **Rendering**:
  - Render each cell as a rectangle with `DrawRectangle(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE, color)`.
  - Suggested `CELL_SIZE = 4` for a 200x150 grid, yielding an 800x600 window.
  - Define colors:
    ```c
    Color material_colors[MATERIAL_COUNT] = {
        SKYBLUE, BLUE, BROWN, YELLOW, GRAY, BEIGE, DARKGRAY, LIGHTGRAY
    };
    ```
  - Example:
    ```c
    void render_grid(Cell *grid) {
        BeginDrawing();
        ClearBackground(BLACK);
        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                int idx = y * GRID_WIDTH + x;
                DrawRectangle(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE,
                              material_colors[grid[idx].material]);
            }
        }
        EndDrawing();
    }
    ```
- **Window Setup**:
  - Initialize with `InitWindow(GRID_WIDTH * CELL_SIZE, GRID_HEIGHT * CELL_SIZE, "Falling Sand Simulation")`.
  - Set `SetTargetFPS(60)` for smooth updates.

## User Interaction
- Allow users to add materials via mouse input:
  - Left-click to place water with 0.05 soil fraction.
  - Example:
    ```c
    void handle_input(Cell *grid) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse = GetMousePosition();
            int x = mouse.x / CELL_SIZE;
            int y = mouse.y / CELL_SIZE;
            if (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT) {
                int idx = y * GRID_WIDTH + x;
                grid[idx].material = WATER;
                grid[idx].carried_fractions[SOIL] = 0.05f;
                grid[idx].density = base_density[WATER] + 0.05f * base_density[SOIL];
            }
        }
    }
    ```

## Simulation Loop
- Main loop:
  1. Handle user input.
  2. Update simulation (call functions in order: density, air swap, water movement, soil/sand movement, sandstone movement, erosion, sedimentation, compaction, evaporation).
  3. Render grid.
  - Example:
    ```c
    int main() {
        srand(time(NULL));
        InitWindow(GRID_WIDTH * CELL_SIZE, GRID_HEIGHT * CELL_SIZE, "Falling Sand Simulation");
        SetTargetFPS(60);
        // Assume grid initialization
        while (!WindowShouldClose()) {
            handle_input(grid_current);
            compute_density(grid_current);
            air_density_swap(grid_current, grid_next);
            Cell *temp = grid_current; grid_current = grid_next; grid_next = temp;
            water_movement(grid_current, grid_next);
            temp = grid_current; grid_current = grid_next; grid_next = temp;
            soil_sand_movement(grid_current, grid_next);
            temp = grid_current; grid_current = grid_next; grid_next = temp;
            sandstone_movement(grid_current, grid_next);
            temp = grid_current; grid_current = grid_next; grid_next = temp;
            erosion(grid_current, grid_next);
            temp = grid_current; grid_current = grid_next; grid_next = temp;
            sedimentation(grid_current, grid_next);
            temp = grid_current; grid_current = grid_next; grid_next = temp;
            compaction(grid_current, grid_next);
            temp = grid_current; grid_current = grid_next; grid_next = temp;
            evaporation(grid_current, grid_next);
            temp = grid_current; grid_current = grid_next; grid_next = temp;
            render_grid(grid_current);
        }
        // Free grids
        free(grid_current);
        free(grid_next);
        CloseWindow();
        return 0;
    }
    ```

## Initialization
- Initialize grids with air (`material = AIR`, `density = 1.0`, zero fractions).
- Add initial materials for testing (e.g., soil at bottom, water above):
  ```c
  void init_grid(Cell *grid) {
      for (int y = 0; y < GRID_HEIGHT; y++) {
          for (int x = 0; x < GRID_WIDTH; x++) {
              int idx = y * GRID_WIDTH + x;
              grid[idx].material = AIR;
              memset(grid[idx].carried_fractions, 0, sizeof(float) * 4);
              grid[idx].density = base_density[AIR];
          }
      }
      // Example: Soil at bottom
      for (int x = 50; x < 150; x++) {
          int idx = (GRID_HEIGHT - 1) * GRID_WIDTH + x;
          grid[idx].material = SOIL;
          grid[idx].density = base_density[SOIL];
      }
      // Example: Water layer
      for (int x = 80; x < 120; x++) {
          int idx = (GRID_HEIGHT - 10) * GRID_WIDTH + x;
          grid[idx].material = WATER;
          grid[idx].density = base_density[WATER];
      }
  }
  ```

## Randomness
- Use `rand()` for probabilistic rules (e.g., sandstone movement, erosion, compaction).
- Initialize with `srand(time(NULL))` at program start.
- Example for 50% chance:
  ```c
  if (rand() % 100 < 50) { /* Action */ }
  ```

## Integration with Existing Code
- **Assumptions**: Your support code handles Raylib setup, window management, and possibly grid allocation.
- **Adaptation**:
  - Integrate the provided `Cell` struct and material enum into your codebase.
  - Use the capacity and density arrays as defined.
  - Implement the simulation functions (`compute_density`, `air_density_swap`, etc.) as separate functions or inline them into your update loop.
  - Call functions in the specified order, swapping grids after each step.
  - Adapt rendering to your Raylib setup, using the provided color array or your own.
  - Add user input handling as needed, using the example as a guide.
- **Memory Management**: Ensure grids are allocated and freed appropriately, matching your existing memory handling.
- **Performance**: For large grids, consider processing in chunks or optimizing loops if performance is an issue. The sequential nature suits smaller grids (e.g., 200x150).

## Notes
- **Performance**: CPU execution is slower than CUDA but sufficient for small to medium grids. Avoid very large grids (e.g., >1000x1000) without optimization.
- **Scalability**: If needed, add multithreading (e.g., via pthreads) to parallelize updates, but this complicates grid swapping.
- **Debugging**: Use printf or Raylib’s `DrawText` to display cell states for debugging.
- **Extensions**: Add more materials, complex interactions, or UI elements (e.g., material selection) based on your support code.
- **Compilation**: Ensure Raylib is linked (e.g., `gcc -o sim sim.c -lraylib -lGL -lm -lpthread -ldl -lrt -lX11` on Linux).

This rule set provides a complete framework for a falling sand simulation in C with Raylib, designed to integrate with your existing support code while preserving the core mechanics of the original CUDA-based design.