#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

// Constants
#define GRID_WIDTH 120
#define GRID_HEIGHT 75
#define CELL_SIZE 10

// Material types
typedef enum {
    MATERIAL_AIR = 0,
    MATERIAL_SAND = 1
} MaterialType;

// Cell structure containing all cell properties
typedef struct {
    MaterialType material;
    float pressure;      // Air pressure (0.0 to 1.0, with 0.5 being neutral)
    float velocity_x;    // For sand movement
    float velocity_y;    // For sand movement
    float temperature;   // Temperature in Celsius (range -50 to 150)
} Cell;

#endif // TYPES_H
