/**
 * @file debug_utils.h
 * @brief Utility functions for debugging the falling sand simulation
 */

#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include "grid.h"
#include <stdio.h>
#include <stdbool.h>

/**
 * @brief Get a character representation of a material (for ASCII debugging)
 * 
 * @param material The CellMaterial to get a character for
 * @return char Character representing the material
 */
char get_material_char(CellMaterial material);

/**
 * @brief Print grid state to a file stream in a readable format
 * 
 * @param grid Pointer to the grid
 * @param stream File stream to print to (e.g., stdout)
 * @param region_x Starting X coordinate of the region to print
 * @param region_y Starting Y coordinate of the region to print
 * @param width Width of the region to print
 * @param height Height of the region to print
 */
void print_grid_to_stream(Grid* grid, FILE* stream, int region_x, int region_y, int width, int height);

/**
 * @brief Print grid state to console
 * 
 * @param grid Pointer to the grid
 * @param region_x Starting X coordinate of the region to print
 * @param region_y Starting Y coordinate of the region to print
 * @param width Width of the region to print
 * @param height Height of the region to print
 */
void print_grid_console(Grid* grid, int region_x, int region_y, int width, int height);

/**
 * @brief Save grid state to a JSON file
 * 
 * @param grid Pointer to the grid
 * @param filename Name of the file to save to
 * @return bool True if successful, false otherwise
 */
bool save_grid_to_file(Grid* grid, const char* filename);

/**
 * @brief Save a specific region of the grid state to a JSON file
 * 
 * @param grid Pointer to the grid
 * @param filename Name of the file to save to
 * @param region_x Starting X coordinate of the region to save
 * @param region_y Starting Y coordinate of the region to save
 * @param width Width of the region to save
 * @param height Height of the region to save
 * @return bool True if successful, false otherwise
 */
bool save_grid_region_to_file(Grid* grid, const char* filename, int region_x, int region_y, int width, int height);

/**
 * @brief Save cell details to a JSON file
 * 
 * @param grid Pointer to the grid
 * @param filename Name of the file to save to
 * @param x X coordinate of the cell
 * @param y Y coordinate of the cell
 * @return bool True if successful, false otherwise
 */
bool save_cell_details_to_file(Grid* grid, const char* filename, int x, int y);

/**
 * @brief Save simulation statistics to a JSON file
 * 
 * @param grid Pointer to the grid
 * @param filename Name of the file to save to
 * @return bool True if successful, false otherwise
 */
bool save_simulation_stats_to_file(Grid* grid, const char* filename);

#endif // DEBUG_UTILS_H
