/**
 * @file grid.h
 * @brief Defines the grid structure and related functions for the falling sand simulation
 */

#ifndef GRID_H
#define GRID_H

#include <stdint.h>
#include <stdbool.h>

// Grid dimensions
#define GRID_WIDTH 200
#define GRID_HEIGHT 150

// Cell material types
typedef enum {
    MATERIAL_AIR = 0,
    MATERIAL_SAND,
    MATERIAL_WATER,
    MATERIAL_SOIL,
    MATERIAL_STONE,
    MATERIAL_WOOD,
    MATERIAL_FIRE,
    MATERIAL_STEAM,
    MATERIAL_ICE,
    MATERIAL_OIL,
    // Add more materials as needed
    MATERIAL_COUNT
} CellMaterial;

// Material properties struct
typedef struct {
    float density;          // How dense/heavy the material is (affects falling behavior)
    float viscosity;        // How quickly the material flows/spreads
    float temperature;      // Default temperature of the material
    bool flammable;         // Whether the material can catch fire
    float heat_conductivity; // How well the material conducts heat
    bool can_carry_materials; // Whether the material can carry other materials (like water or air)
    uint32_t color;         // Base color for rendering
} MaterialProperties;

// Cell structure
typedef struct {
    CellMaterial material;          // Current primary material in this cell
    float density;                  // Current density (can change based on pressure, etc.)
    float temperature;              // Current temperature of the cell
    float pressure;                 // Current pressure at this cell
    uint8_t material_quantities[MATERIAL_COUNT]; // Fraction of each material in the cell (mainly for air/water carrying others)
    bool updated;                   // Flag to track if cell was updated in current simulation step
} Cell;

// Grid structure
typedef struct {
    Cell* cells;             // 2D array of cells flattened to 1D
    uint32_t width;          // Width of the grid
    uint32_t height;         // Height of the grid
    uint32_t frame_counter;  // Current simulation frame counter
} Grid;

/**
 * @brief Initialize the grid with default values
 * 
 * @return Grid* Pointer to the initialized grid
 */
Grid* grid_init(void);

/**
 * @brief Free the memory allocated for the grid
 * 
 * @param grid Pointer to the grid to be freed
 */
void grid_free(Grid* grid);

/**
 * @brief Get a cell at specific coordinates
 * 
 * @param grid Pointer to the grid
 * @param x X coordinate
 * @param y Y coordinate
 * @return Cell* Pointer to the cell, or NULL if out of bounds
 */
Cell* grid_get_cell(Grid* grid, int x, int y);

/**
 * @brief Set material at specific coordinates
 * 
 * @param grid Pointer to the grid
 * @param x X coordinate
 * @param y Y coordinate
 * @param material CellMaterial to set
 * @return bool True if successful, false if coordinates are out of bounds
 */
bool grid_set_material(Grid* grid, int x, int y, CellMaterial material);

/**
 * @brief Reset the "updated" flag for all cells in the grid
 * 
 * @param grid Pointer to the grid
 */
void grid_reset_updates(Grid* grid);

/**
 * @brief Get the material properties for a specific material
 * 
 * @param material CellMaterial type
 * @return MaterialProperties Properties of the material
 */
MaterialProperties grid_get_material_properties(CellMaterial material);

/**
 * @brief Check if coordinates are within grid bounds
 * 
 * @param grid Pointer to the grid
 * @param x X coordinate
 * @param y Y coordinate
 * @return bool True if in bounds, false otherwise
 */
bool grid_in_bounds(Grid* grid, int x, int y);

/**
 * @brief Get the name of a material as a string
 * 
 * @param material CellMaterial type
 * @return const char* String representation of the material
 */
const char* grid_material_name(CellMaterial material);

#endif // GRID_H
