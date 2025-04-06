#ifndef UPDATECELLS_H
#define UPDATECELLS_H

#include "cell_types.h"
#include "grid.h"

// Direction bit flags for efficient direction handling
#define DIR_UP_LEFT    0x01  // 00000001
#define DIR_UP         0x02  // 00000010
#define DIR_UP_RIGHT   0x04  // 00000100
#define DIR_LEFT       0x08  // 00001000
#define DIR_RIGHT      0x10  // 00010000
#define DIR_DOWN_LEFT  0x20  // 00100000
#define DIR_DOWN       0x40  // 01000000
#define DIR_DOWN_RIGHT 0x80  // 10000000

// Direction arrays for easy iteration through neighbors
extern const int DIR_X[8];
extern const int DIR_Y[8];

// Main update function to replace UpdateGrid
void updateCells(void);

// Cell type specific update functions
void updateCell(int x, int y);
void updateSoilCell(int x, int y);
void updateWaterCell(int x, int y);
void updateAirCell(int x, int y);
void updatePlantCell(int x, int y);
void updateMossCell(int x, int y);

// Helper functions
unsigned char getValidDirections(int x, int y, int cellType);
unsigned char getEmptyDirections(int x, int y);
unsigned char getMoistureDirections(int x, int y, int threshold);
bool tryMoveInDirection(int x, int y, unsigned char direction);
bool tryMoistureDiffusion(int x, int y, unsigned char direction, int amount);

#endif // UPDATECELLS_H