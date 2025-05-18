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
extern const float BASE_DENSITIES[6];

// Helper functions
int random_chance(void);
float calculate_density(GridCell* cell);
void swap_cells(int x1, int y1, int x2, int y2);
void push_cellvoid(int x1, int y1, int x2, int y2);
void fill_empty_with_air(int x, int y);

// Cell-type specific update functions
void update_air(void);
void update_water(void);
void update_soil(void);
void update_moss(void);
void update_plant(void);
void update_erosion_sedimentation(void);
void update_evaporation_precipitation(void);
void process_cells_below_border(void);

// Main update function to replace UpdateGrid
void updateCells(void);

#endif // UPDATECELLS_H