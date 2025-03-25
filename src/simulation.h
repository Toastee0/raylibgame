#ifndef SIMULATION_H
#define SIMULATION_H

#include "grid.h"
#include <stdbool.h>

// Main grid update function
void UpdateGrid(void);

// Cell-type specific update functions
void UpdateSoil(void);
void UpdateWater(void);
void UpdateAir(void);
void UpdateEvaporation(void);

// Helper functions
void UpdateAirColor(int x, int y);
void MergeAirMoisture(int x, int y);
int CountWaterNeighbors(int x, int y);

// Temperature functions
void InitializeTemperature(void);

#endif // SIMULATION_H
