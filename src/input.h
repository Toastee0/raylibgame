/**
 * @file input.h
 * @brief Functions for handling user input
 */

#ifndef INPUT_H
#define INPUT_H

#include "grid.h"

/**
 * @brief Initialize input handling
 * 
 * @return bool True if successful, false otherwise
 */
bool input_init(void);

/**
 * @brief Process user input
 * 
 * @param grid Pointer to the grid
 * @param selected_material Pointer to the currently selected material
 * @param brush_size Pointer to the current brush size
 * @param paused Pointer to the simulation pause state
 * @param show_debug Pointer to the debug mode state
 * @return bool True if the application should continue, false if it should exit
 */
bool input_process(Grid* grid, CellMaterial* selected_material, int* brush_size, bool* paused, bool* show_debug);

/**
 * @brief Get the position of the mouse in grid coordinates
 * 
 * @param grid_x Pointer to store the X coordinate
 * @param grid_y Pointer to store the Y coordinate
 */
void input_get_mouse_grid_pos(int* grid_x, int* grid_y);

#endif // INPUT_H
