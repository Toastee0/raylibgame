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

## Contributing

*(Details to be added if collaboration is planned)*
*   Check the Issues tab on GitHub for current tasks and bugs.
*   Follow existing coding conventions.
*   Create feature branches for new work.
