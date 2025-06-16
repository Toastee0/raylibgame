#ifndef GRID_H
#define GRID_H

#include "types.h"

// Grid management functions
void InitializeGrid(void);
Cell* GetGrid(void);
Cell* GetNextGrid(void);
void SwapGrids(void);
bool IsValidPosition(int x, int y);

#endif // GRID_H
