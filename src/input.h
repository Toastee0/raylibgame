#ifndef INPUT_H
#define INPUT_H

#include "types.h"

// Input handling functions
void HandleInput(void);
void AddSand(int gridX, int gridY, int radius);
void AddHeat(int gridX, int gridY, int radius);
bool GetShowTemperature(void);

#endif // INPUT_H
