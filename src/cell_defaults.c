#include "cell_defaults.h"
#include "raylib.h"

void InitializeCellDefaults(GridCell* cell, int type) {
    // Common defaults
    cell->type = type;
    cell->objectID = 0;
    cell->position = (Vector2){0, 0};
    cell->origin = (Vector2){0, 0};
    cell->is_falling = false;
    
    // Type-specific defaults
    switch(type) {
        case CELL_TYPE_BORDER:
            cell->baseColor = GRAY;
            cell->colorhigh = 0;
            cell->colorlow = 0;
            cell->volume = 10;
            cell->Energy = 0;
            cell->height = 0;
            cell->moisture = 0;
            cell->desiredmoisture = 0;
            cell->permeable = 0;
            cell->age = 0;
            cell->maxage = 0;
            cell->temperature = 20;
            cell->freezingpoint = 0;
            cell->boilingpoint = 100;
            cell->temperaturepreferanceoffset = 0;
            break;
            
        case CELL_TYPE_AIR:
            cell->baseColor = WHITE;
            cell->colorhigh = 255;
            cell->colorlow = 200;
            cell->volume = 1;
            cell->Energy = 0;
            cell->height = 0;
            cell->moisture = 20;
            cell->desiredmoisture = 20;
            cell->permeable = 1;
            cell->age = 0;
            cell->maxage = 0;
            cell->temperature = 20;
            cell->freezingpoint = 0;
            cell->boilingpoint = 100;
            cell->temperaturepreferanceoffset = 0;
            break;
            
        case CELL_TYPE_SOIL:
            cell->baseColor = (Color){127, 106, 79, 255};  // Brown
            cell->colorhigh = 0;
            cell->colorlow = 0;
            cell->volume = 7;
            cell->Energy = 0;
            cell->height = 0;
            cell->moisture = 50;
            cell->desiredmoisture = 50;
            cell->permeable = 1;
            cell->age = 0;
            cell->maxage = 0;
            cell->temperature = 20;
            cell->freezingpoint = 0;
            cell->boilingpoint = 200;
            cell->temperaturepreferanceoffset = 0;
            break;
            
        case CELL_TYPE_WATER:
            cell->baseColor = BLUE;
            cell->colorhigh = 0;
            cell->colorlow = 0;
            cell->volume = 10;
            cell->Energy = 0;
            cell->height = 0;
            cell->moisture = 100;
            cell->desiredmoisture = 100;
            cell->permeable = 1;
            cell->age = 0;
            cell->maxage = 0;
            cell->temperature = 20;
            cell->freezingpoint = 0;
            cell->boilingpoint = 100;
            cell->temperaturepreferanceoffset = 0;
            break;
            
        case CELL_TYPE_PLANT:
            cell->baseColor = GREEN;
            cell->colorhigh = 200;
            cell->colorlow = 100;
            cell->volume = 5;
            cell->Energy = 5;
            cell->height = 0;
            cell->moisture = 50;
            cell->desiredmoisture = 70;
            cell->permeable = 0;
            cell->age = 0;
            cell->maxage = 1000;
            cell->temperature = 20;
            cell->freezingpoint = 0;
            cell->boilingpoint = 200;
            cell->temperaturepreferanceoffset = 5;
            break;

        case CELL_TYPE_ROCK:
            cell->baseColor = DARKGRAY;
            cell->colorhigh = 0;
            cell->colorlow = 0;
            cell->volume = 10;
            cell->Energy = 0;
            cell->height = 0;
            cell->moisture = 0;
            cell->desiredmoisture = 0;
            cell->permeable = 0;
            cell->age = 0;
            cell->maxage = 0;
            cell->temperature = 20;
            cell->freezingpoint = 0;
            cell->boilingpoint = 1000;
            cell->temperaturepreferanceoffset = 0;
            break;

        case CELL_TYPE_MOSS:
            cell->baseColor = DARKGREEN;
            cell->colorhigh = 100;
            cell->colorlow = 50;
            cell->volume = 3;
            cell->Energy = 3;
            cell->height = 0;
            cell->moisture = 70;
            cell->desiredmoisture = 80;
            cell->permeable = 1;
            cell->age = 0;
            cell->maxage = 500;
            cell->temperature = 20;
            cell->freezingpoint = 0;
            cell->boilingpoint = 100;
            cell->temperaturepreferanceoffset = 0;
            break;
    }
}