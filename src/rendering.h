#ifndef RENDERING_H
#define RENDERING_H

#include "types.h"
#include "raylib.h"

// Rendering functions
void DrawSimulationGrid(void);
void DrawUI(void);
Color GetCellColor(Cell cell);

#endif // RENDERING_H
