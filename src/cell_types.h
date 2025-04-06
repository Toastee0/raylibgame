#ifndef CELL_TYPES_H
#define CELL_TYPES_H

#include "raylib.h"

// Cell type constants
#define CELL_TYPE_BORDER -1 // immutable border
#define CELL_TYPE_AIR 0 // contains moisture, also acts as the gas form of water. amount of moisture drives how white it is.
#define CELL_TYPE_SOIL 1 // contains moisture, plants can grow here, water can be shed or absorbed. amount of moisture drives how dark brown it is.
#define CELL_TYPE_WATER 2 //liquid water, amount of moisture drives how blue it is.
#define CELL_TYPE_PLANT 3 //plant, green varies a litte bit randomly as it grows to provide variation
#define CELL_TYPE_ROCK 4 // rock, grey, cannot be moved. does not absorb moisture.
#define CELL_TYPE_MOSS 5 // dark green, uses moisture, grows on soil. essentially green soil, but clumpy.

typedef struct {
    int type;  // -1 = immutable border 0 = air, 1 = soil, 2 = water, 3 = plant, 4 = vapor
    int objectID; //unique identifier for the object or plant 
    Vector2 position;
    Vector2 origin; //co ordinates of the first pixel of the object or plant, if a multi pixel object.
    Color baseColor; //basic color of the pixel
    int volume; //1-10, how much of the density of the object is filled, 1 = 10% 10 = 100%, for allowing water to evaoprate into moist air, or be absorbed by soil.
    int Energy; //5 initial, reduced when replicating.
    int height; //height of the pixel, intially 0, this is an offset to allow limiting and guiding the growth of plant type pixels.
    int moisture; // Moisture level: 0-100 integer, 0 = dry, 100 = saturated
    int desiredmoisture; //desired moisture level, used to guide the movement of water. 50 for sand, 100 for water, 20 for air.
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
