#ifndef GRID_H
#define GRID_H

#include <stdbool.h>
#include "cell_types.h"

// Define grid dimensions and cell size (accessible globally)
extern int GRID_WIDTH;
extern int GRID_HEIGHT;
extern int CELL_SIZE;

// Grid structure
extern GridCell** grid;

// Initialize the grid
void InitGrid(void);

// Clean up grid memory when program ends
void CleanupGrid(void);

// Check if a tile is a border or out of bounds
bool IsBorderTile(int x, int y);

// Check if we can move to a tile
bool CanMoveTo(int x, int y);

// Save grid to file - returns true if successful
bool SaveGridToFile(const char* filename);

// Load grid from file - returns true if successful
bool LoadGridFromFile(const char* filename);

#endif // GRID_H
