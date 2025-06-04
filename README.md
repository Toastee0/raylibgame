# Raylib Falling Sand Ecosystem Simulation

A 2D falling sand-style ecosystem simulation game built with C and Raylib.

## Project Vision

This project aims to create a simulated ecosystem where the game field is divided into cells. The behavior and interaction of materials within these cells are governed by a set of rules, leading to an emergent, dynamic environment. The simulation will include concepts such as:

*   **Cellular Automata:** The grid is composed of cells whose properties change based on their own state and the state of their neighbors.
*   **Material Properties:** Different materials (air, water, soil, minerals, etc.) with unique characteristics.
*   **Physics Simulation:** Including pressure, temperature, density.
*   **Phase Changes:** Water can exist as liquid, ice (freeze), or steam (boil).
*   **Atmospheric Effects:** Evaporation, condensation, rain, and clouds.
*   **Material Transport:** Air and water can carry other materials like minerals.
*   **Dynamic Rendering:** Cell colors and appearance will change based on their contents and interactions, creating an animated effect.
*   **Gas/Liquid Dynamics:** Inspired by games like "Oxygen Not Included," with considerations for pressure wakes and air compression.

## Core Components

1.  **Simulation Engine:** Manages the grid, cell states, and applies the rules for material interaction and environmental changes.
2.  **Rendering Engine:** Uses Raylib to draw the current state of the simulation grid to the screen.
3.  **User Interface (UI):** Allows user interaction with the simulation (e.g., adding materials, controlling simulation speed).
4.  **Debug Output:** A JSON-based output of the game field to facilitate debugging and analysis, potentially by AI agents.

## Grid System

*   The game world is a 2D grid of cells.
*   Cells themselves are fixed in position, but their properties (material type, density, temperature, carried fractions, etc.) are dynamic.
*   Cell size can be configurable (e.g., 8x8 pixels or larger) to support various display resolutions and performance targets.

## Planned Features (High-Level)

*   Basic falling sand mechanics for solid materials.
*   Water flow, pressure, and buoyancy.
*   Temperature simulation affecting materials (freezing, boiling).
*   Air pressure and basic gas flow.
*   Evaporation and condensation cycle.
*   Mineral transport and deposition.
*   User tools to add/remove different materials.
*   A debug view to inspect cell data.

## Building and Running

*(Instructions to be refined as the project evolves)*

### Prerequisites

*   A C compiler (e.g., MinGW for Windows, GCC for Linux)
*   Raylib library (ensure `RAYLIB_PATH` environment variable or Makefile variable is set correctly, e.g., `C:/raylib/raylib` on Windows)

### Build Steps (using Makefile)

1.  Clone the repository.
2.  Navigate to the project directory in your terminal.
3.  **For a debug build:**
    ```sh
    mingw32-make.exe RAYLIB_PATH=C:/raylib/raylib BUILD_MODE=DEBUG
    ```
4.  **For a release build:**
    ```sh
    mingw32-make.exe RAYLIB_PATH=C:/raylib/raylib
    ```
5.  Run the compiled executable (e.g., `game.exe` on Windows).

Alternatively, use the provided VS Code tasks:
*   `build debug`
*   `build release`

## Code Architecture and Implementation Details

### Core Files and Their Roles

1. **grid.h/grid.c**
   - Defines the `Grid`, `Cell`, and `CellMaterial` (enum) structures
   - Manages the 2D grid of cells and their properties
   - Provides material properties for each material type
   - Contains utility functions for accessing cells and manipulating the grid

2. **simulation.h/simulation.c**
   - Implements the core simulation logic
   - Contains material-specific update functions (solids, liquids, gases)
   - Handles temperature changes, phase transitions, and material transport
   - Fire propagation and combustion simulation

3. **rendering.h/rendering.c**
   - Manages all visualization aspects
   - Converts internal cell states to colors and textures
   - Handles camera controls (zoom, pan)
   - Draws the UI elements and debug information

4. **input.h/input.c**
   - Processes mouse and keyboard input
   - Implements user tools for adding/removing materials
   - Controls brush size, selected material, etc.

5. **color_utils.h/color_utils.c**
   - Provides utilities for color manipulation
   - Converts between different color formats

6. **debug_utils.h/debug_utils.c**
   - Contains tools for debugging the simulation
   - JSON output for inspection by external tools

### Simulation Mechanics

#### Material Types and Properties
- **Air**: Lightweight gas, can carry other materials
- **Sand**: Classic falling particle with medium density
- **Water**: Liquid that flows and can carry materials
- **Soil**: Similar to sand but with different properties
- **Stone**: Static material (doesn't move), useful for building structures and cave systems
- **Wood**: Static material that can catch fire
- **Fire**: Dynamic element that spreads and burns out over time
- **Steam**: Rising gas formed when water evaporates
- **Ice**: Frozen water that can melt back
- **Oil**: Flammable liquid with lower density than water

#### Physics Simulation

1. **Gravity and Material Movement**
   - Solids fall downward based on gravity
   - Liquids flow downward and spread horizontally
   - Gases rise upward due to buoyancy

2. **Temperature System**
   - Heat transfer between adjacent cells
   - Temperature affects material appearance
   - Triggers phase changes (water↔steam↔ice)
   - Can cause combustion of flammable materials

3. **Material Transport**
   - Fluids (air, water) can carry small quantities of other materials
   - Materials settle or disperse based on their properties

4. **Fire and Combustion**
   - Fire spreads to adjacent flammable cells
   - Burns out over time
   - Generates heat affecting surrounding cells

### Implementation Details

- **Cell Updates**: The simulation processes cells from bottom to top for falling materials, alternating left-to-right and right-to-left on even/odd frames for more natural dispersal.

- **Material Interactions**: Each material has defined rules for how it interacts with others (e.g., sand can displace water due to higher density).

- **Update Flags**: Cells track if they've been updated in the current simulation step to prevent multiple updates within the same frame.

- **Rendering Optimization**: Only visible portions of the grid are rendered, with special handling for zooming and panning.

## Extending the Simulation

### Adding New Materials

To add a new material to the simulation:

1. **Update the `CellMaterial` enum in `grid.h`**:
   ```c
   typedef enum {
       // Existing materials...
       MATERIAL_NEW_MATERIAL,
       MATERIAL_COUNT
   } CellMaterial;
   ```

2. **Define material properties in `grid.c`**:
   ```c
   // In the material_properties array
   // MATERIAL_NEW_MATERIAL
   {
       .density = 1.2f,
       .viscosity = 0.0f,
       .temperature = 20.0f,
       .flammable = false,
       .heat_conductivity = 0.3f,
       .can_carry_materials = false,
       .color = 0x123456FF  // RGBA color
   },
   ```

3. **Add the material name in `grid.c`**:
   ```c
   // In the material_names array
   "New Material",
   ```

4. **Implement behavior in `simulation.c`**:
   - Add case handling in the `update_cell` function
   - Create a specialized update function if needed

5. **Update UI elements** in `rendering.c` to include the new material in the selection UI

### Creating Custom Behaviors

To implement custom behaviors for materials:

1. Define a new update function in `simulation.c`:
   ```c
   bool update_custom_material(Grid* grid, int x, int y, Cell* cell) {
       // Custom behavior logic
       return was_updated;
   }
   ```

2. Call this function from the appropriate case in `update_cell`.

## Contributing

*(Details to be added if collaboration is planned)*
*   Check the Issues tab on GitHub for current tasks and bugs.
*   Follow existing coding conventions.
*   Create feature branches for new work.
