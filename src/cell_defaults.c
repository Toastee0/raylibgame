#include "cell_defaults.h"
#include "raylib.h"

void InitializeCellDefaults(GridCell* cell, int type) {
    // Common defaults
    cell->type = type;
    cell->objectID = 0;
    
    cell->updated_this_frame = false;
    cell->is_falling = false;
    
    // Initialize element composition and physical properties
    cell->oxygen = 0;
    cell->water = 0;
    cell->mineral = 0;
    cell->pressure = 0;
    cell->nominal_pressure = 0; // Initialize nominal pressure
    cell->density = 0;
    cell->dewpoint = 0;
    
    // Life cycle and temperature properties
    cell->age = 0;
    cell->maxage = 0;
    cell->temperature = 20;
    cell->freezingpoint = 0;
    cell->boilingpoint = 100;
    
    // Type-specific defaults
    switch(type) {
        case CELL_TYPE_BORDER:
            cell->baseColor = GRAY;
            cell->mineral = 100;
            cell->oxygen = 0;
            cell->water = 0;
            cell->density = 100; // Very dense
            cell->nominal_pressure = 100; // Very high nominal pressure for borders
            cell->freezingpoint = 0;
            cell->boilingpoint = 100;
            break;
            
        case CELL_TYPE_AIR:
            cell->baseColor = WHITE;
            cell->oxygen = 90;
            cell->water = 10;
            cell->mineral = 0;
            cell->pressure = 1; // Standard atmospheric pressure
            cell->nominal_pressure = 1; // Standard atmospheric pressure
            cell->density = 1;  // Low density
            cell->dewpoint = 10; // Dewpoint below ambient temperature
            cell->freezingpoint = -200; // Very low freezing point
            cell->boilingpoint = -180; // Oxygen boiling point
            break;
            
        case CELL_TYPE_SOIL:
            cell->baseColor = (Color){127, 106, 79, 255};  // Brown
            cell->oxygen = 10;
            cell->water = 30;
            cell->mineral = 60;
            cell->density = 20; // Medium density
            cell->nominal_pressure = 20; // Medium nominal pressure
            cell->freezingpoint = 0;
            cell->boilingpoint = 200;
            break;
            
        case CELL_TYPE_WATER:
            cell->baseColor = BLUE;
            cell->oxygen = 5;
            cell->water = 90;
            cell->mineral = 5;
            cell->density = 10; // Standard density for water
            cell->nominal_pressure = 10; // Standard nominal pressure for water
            cell->freezingpoint = 0;
            cell->boilingpoint = 100;
            break;
            
        case CELL_TYPE_PLANT:
            cell->baseColor = GREEN;
            cell->oxygen = 20;
            cell->water = 60;
            cell->mineral = 20;
            cell->density = 5;  // Low-medium density
            cell->nominal_pressure = 5; // Low-medium nominal pressure
            cell->maxage = 1000;
            cell->freezingpoint = 0;
            cell->boilingpoint = 200;
            break;

        case CELL_TYPE_ROCK:
            cell->baseColor = DARKGRAY;
            cell->oxygen = 0;
            cell->water = 0;
            cell->mineral = 100;
            cell->density = 30; // High density
            cell->nominal_pressure = 30; // High nominal pressure
            cell->freezingpoint = 0;
            cell->boilingpoint = 1000;
            break;

        case CELL_TYPE_MOSS:
            cell->baseColor = DARKGREEN;
            cell->oxygen = 15;
            cell->water = 55;
            cell->mineral = 30;
            cell->density = 15; // Between soil and plant density
            cell->nominal_pressure = 15; // Between soil and plant nominal pressure
            cell->maxage = 500;
            cell->freezingpoint = 0;
            cell->boilingpoint = 100;
            break;
    }
}