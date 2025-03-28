#ifndef GRID_H
#define GRID_H

#include "cell_types.h"

// Grid constants
extern  int CELL_SIZE;
extern  int GRID_WIDTH;
extern  int GRID_HEIGHT;

// Grid data
extern GridCell** grid;

// Grid initialization and utility functions
void InitGrid(void);
void CleanupGrid(void);
int CalculateTotalMoisture(void);
int ClampMoisture(int value);
bool IsBorderTile(int x, int y);
bool CanMoveTo(int x, int y);

// Initialize temperature gradient for all cells
void InitializeTemperatureGradient(void);

#endif // GRID_H
