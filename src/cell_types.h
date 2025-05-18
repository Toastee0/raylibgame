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
#define CELL_TYPE_EMPTY 6 // empty, cells will prefer to fall into empty cells. these only exist temporarily in the sim buffer, and are not rendered.


typedef struct {
    int type;  // -1 = immutable border 0 = air, 1 = soil, 2 = water, 3 = plant, 4 = vapor
    int objectID; //unique identifier for the object or plant 
    
    Color baseColor; //basic color of the pixel
    //cells are made up of a ratio of the 3 main elements, oxygen, water, and minerals. the ratio of these elements is used to determine the color of the cell, and it's type for behavior.
    int oxygen; //air is mosty oxygen ,but can carry small amounts of water and mineral.
    int water; //water is mostly water but can carry small amounts of oxygen and minerals.
    int mineral; //soil is mostly minerals, but can carry trap amounts of water and oxygen. rock is very dense soil.
    int nominal_pressure; //the pressure of the cell, used for determining if the cell is a gas or liquid. (or solid) this is used to determine if the cell can be moved into or not.
    int pressure; //used for allowing air to be displaced instead of swapped with other cells.
    int density; //density of the object, used for storing the amount of material packed into the cell.
    int dewpoint; //the temerpature at which the air becomes saturated with moisture, and water condenses out of the air. (or molten rock solidifies)

    
    int age; //age of the object, used for plant growth and reproduction.
    int maxage; //max age of the object, used for plant growth and reproduction.
    int temperature; //temperature of the object.
    int freezingpoint; //freezing point of the object. (or solidicdification point for rocks/soil to not be molten)
    int boilingpoint; //boiling point of the material, water, steam, etc.
    

    bool updated_this_frame; // Add this flag to track updates
    bool is_falling; // New field to explicitly track falling state
    bool empty; //tracks if the cell is empty because it was vacated by a falling object. these cells are not rendered, and should all be resolved to non-empty before we push the pixels to the second buffer.
} GridCell;

#endif // CELL_TYPES_H
