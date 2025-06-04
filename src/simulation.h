/**
 * @file simulation.h
 * @brief Functions for the falling sand simulation
 */

#ifndef SIMULATION_H
#define SIMULATION_H

#include "grid.h"

/**
 * @brief Initialize the simulation
 * 
 * @return bool True if successful, false otherwise
 */
bool simulation_init(void);

/**
 * @brief Clean up the simulation
 */
void simulation_cleanup(void);

/**
 * @brief Update all cells in the grid for one simulation step
 * 
 * @param grid Pointer to the grid
 */
void simulation_update(Grid* grid);

/**
 * @brief Add some material to the grid at specified location with a given radius
 * 
 * @param grid Pointer to the grid
 * @param x X coordinate center
 * @param y Y coordinate center
 * @param radius Radius of the circle to fill
 * @param material CellMaterial to add
 */
void simulation_add_material(Grid* grid, int x, int y, int radius, CellMaterial material);

/**
 * @brief Update solids physics (like sand falling)
 * 
 * @param grid Pointer to the grid
 * @param x X coordinate of the cell
 * @param y Y coordinate of the cell
 * @param cell Pointer to the cell
 * @return bool True if the cell was updated, false otherwise
 */
bool update_solid(Grid* grid, int x, int y, Cell* cell);

/**
 * @brief Update liquids physics (like water flowing)
 * 
 * @param grid Pointer to the grid
 * @param x X coordinate of the cell
 * @param y Y coordinate of the cell
 * @param cell Pointer to the cell
 * @return bool True if the cell was updated, false otherwise
 */
bool update_liquid(Grid* grid, int x, int y, Cell* cell);

/**
 * @brief Update gas physics (like steam rising)
 * 
 * @param grid Pointer to the grid
 * @param x X coordinate of the cell
 * @param y Y coordinate of the cell
 * @param cell Pointer to the cell
 * @return bool True if the cell was updated, false otherwise
 */
bool update_gas(Grid* grid, int x, int y, Cell* cell);

/**
 * @brief Update temperature effects (heat transfer, phase changes)
 * 
 * @param grid Pointer to the grid
 */
void update_temperature(Grid* grid);

/**
 * @brief Update material transport (carrying fractions of materials)
 * 
 * @param grid Pointer to the grid
 */
void update_material_transport(Grid* grid);

/**
 * @brief Update fire and combustion effects
 * 
 * @param grid Pointer to the grid 
 */
void update_fire(Grid* grid);

/**
 * @brief Update a single cell based on its material type
 * 
 * @param grid Pointer to the grid
 * @param x X coordinate of the cell
 * @param y Y coordinate of the cell
 */
void update_cell(Grid* grid, int x, int y);

/**
 * @brief Update a fire cell (spreading, burning out)
 * 
 * @param grid Pointer to the grid
 * @param x X coordinate of the cell
 * @param y Y coordinate of the cell
 * @param cell Pointer to the cell
 * @return bool True if the cell was updated, false otherwise
 */
bool update_fire_cell(Grid* grid, int x, int y, Cell* cell);

#endif // SIMULATION_H
