#ifndef CELL_TYPES_H
#define CELL_TYPES_H

#include "raylib.h"

// Cell type constants
#define CELL_TYPE_BORDER -1
#define CELL_TYPE_AIR 0
#define CELL_TYPE_SOIL 1
#define CELL_TYPE_WATER 2
#define CELL_TYPE_PLANT 3
#define CELL_TYPE_VAPOR 4

typedef struct {
    int type;  // -1 = immutable border 0 = air, 1 = soil, 2 = water, 3 = plant, 4 = vapor
    int objectID; //unique identifier for the object or plant 
    Vector2 position;
    Vector2 origin; //co ordinates of the first pixel of the object or plant, if a multi pixel object.
    Color baseColor; //basic color of the pixel
    int colorhigh; // max variation of color for the pixel
    int colorlow; // min variation of color for the pixel
    int volume; //1-10, how much of the density of the object is filled, 1 = 10% 10 = 100%, for allowing water to evaoprate into moist air, or be absorbed by soil.
    int Energy; //5 initial, reduced when replicating.
    int height; //height of the pixel, intially 0, this is an offset to allow limiting and guiding the growth of plant type pixels.
    int moisture; // Moisture level: 0-100 integer instead of 0.0-1.0 float
    int permeable; //0 = impermeable, 1 = permeable (water permeable)
    int age; //age of the object, used for plant growth and reproduction.
    int maxage; //max age of the object, used for plant growth and reproduction.
    int temperature; //temperature of the object.
    int freezingpoint; //freezing point of the object.
    int boilingpoint; //boiling point of the object.
    int temperaturepreferanceoffset; 
    bool is_falling; // New field to explicitly track falling state
} GridCell;

#endif // CELL_TYPES_H
