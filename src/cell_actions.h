#ifndef CELL_ACTIONS_H
#define CELL_ACTIONS_H

#include "raylib.h"
#include "grid.h"

// Function to place soil
void PlaceSoil(Vector2 position);

// Function to place water
void PlaceWater(Vector2 position);

// Function to place plant
void PlacePlant(Vector2 position);

// Function to place rock
void PlaceRock(Vector2 position);

// Function to place moss
void PlaceMoss(Vector2 position);

// Function to place air
void PlaceAir(Vector2 position);

// Function to place cells in a circular pattern
void PlaceCircularPattern(int centerX, int centerY, int cellType, int radius);

// Function to move a cell from one position to another
void MoveCell(int x1, int y1, int x2, int y2);

// Swap the contents of two cells
void SwapCells(int x1, int y1, int x2, int y2);

#endif // CELL_ACTIONS_H
