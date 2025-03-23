#ifndef CELL_ACTIONS_H
#define CELL_ACTIONS_H

#include "raylib.h"

// Cell creation
void PlaceSoil(Vector2 position);
void PlaceWater(Vector2 position);
void PlaceVapor(Vector2 position, int moisture);
void PlaceCircularPattern(int centerX, int centerY, int cellType, int radius);

// Cell movement
void MoveCell(int x, int y, int x2, int y2);

#endif // CELL_ACTIONS_H
