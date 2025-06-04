/**
 * @file grid.c
 * @brief Implementation of the grid structure and related functions
 */

#include "grid.h"
#include <stdlib.h>
#include <string.h>
#include <raylib.h>

// Define material properties
static MaterialProperties material_properties[MATERIAL_COUNT] = {
    // MATERIAL_AIR
    {
        .density = 0.0f,
        .viscosity = 0.0f,
        .temperature = 20.0f,
        .flammable = false,
        .heat_conductivity = 0.1f,
        .can_carry_materials = true,
        .color = 0x000000FF  // Transparent or black
    },
    // MATERIAL_SAND
    {
        .density = 1.5f,
        .viscosity = 0.0f,
        .temperature = 20.0f,
        .flammable = false,
        .heat_conductivity = 0.2f,
        .can_carry_materials = false,
        .color = 0xC2B280FF  // Sand color
    },
    // MATERIAL_WATER
    {
        .density = 1.0f,
        .viscosity = 0.7f,
        .temperature = 20.0f,
        .flammable = false,
        .heat_conductivity = 0.6f,
        .can_carry_materials = true,
        .color = 0x0000AAFF  // Blue
    },
    // MATERIAL_SOIL
    {
        .density = 1.3f,
        .viscosity = 0.0f,
        .temperature = 20.0f,
        .flammable = false,
        .heat_conductivity = 0.3f,
        .can_carry_materials = false,
        .color = 0x5D4037FF  // Brown
    },
    // MATERIAL_STONE
    {
        .density = 2.5f,
        .viscosity = 0.0f,
        .temperature = 20.0f,
        .flammable = false,
        .heat_conductivity = 0.4f,
        .can_carry_materials = false,
        .color = 0x757575FF  // Gray
    },
    // MATERIAL_WOOD
    {
        .density = 0.8f,
        .viscosity = 0.0f,
        .temperature = 20.0f,
        .flammable = true,
        .heat_conductivity = 0.2f,
        .can_carry_materials = false,
        .color = 0x8B4513FF  // SaddleBrown
    },
    // MATERIAL_FIRE
    {
        .density = 0.1f,
        .viscosity = 0.0f,
        .temperature = 800.0f,
        .flammable = false,
        .heat_conductivity = 1.0f,
        .can_carry_materials = false,
        .color = 0xFF4500FF  // OrangeRed
    },
    // MATERIAL_STEAM
    {
        .density = 0.05f,
        .viscosity = 0.1f,
        .temperature = 110.0f,
        .flammable = false,
        .heat_conductivity = 0.4f,
        .can_carry_materials = true,
        .color = 0xE0E0E0FF  // Light gray
    },
    // MATERIAL_ICE
    {
        .density = 0.9f,
        .viscosity = 0.0f,
        .temperature = -10.0f,
        .flammable = false,
        .heat_conductivity = 0.3f,
        .can_carry_materials = false,
        .color = 0xADD8E6FF  // Light blue
    },
    // MATERIAL_OIL
    {
        .density = 0.8f,
        .viscosity = 0.5f,
        .temperature = 20.0f,
        .flammable = true,
        .heat_conductivity = 0.1f,
        .can_carry_materials = true,
        .color = 0x3C3020FF  // Dark brown
    }
};

// Material names (for debug output)
static const char* material_names[MATERIAL_COUNT] = {
    "Air",
    "Sand",
    "Water",
    "Soil",
    "Stone",
    "Wood",
    "Fire",
    "Steam",
    "Ice",
    "Oil"
};

Grid* grid_init(void) {
    Grid* grid = (Grid*)malloc(sizeof(Grid));
    if (!grid) return NULL;
    
    grid->width = GRID_WIDTH;
    grid->height = GRID_HEIGHT;
    grid->frame_counter = 0;
    
    // Allocate cells
    grid->cells = (Cell*)malloc(sizeof(Cell) * GRID_WIDTH * GRID_HEIGHT);
    if (!grid->cells) {
        free(grid);
        return NULL;
    }
    
    // Initialize all cells to air
    for (uint32_t y = 0; y < GRID_HEIGHT; y++) {
        for (uint32_t x = 0; x < GRID_WIDTH; x++) {
            Cell* cell = grid_get_cell(grid, x, y);
            cell->material = MATERIAL_AIR;
            cell->density = material_properties[MATERIAL_AIR].density;
            cell->temperature = material_properties[MATERIAL_AIR].temperature;
            cell->pressure = 0.0f;
            cell->updated = false;
            
            // Initialize all material quantities to 0
            memset(cell->material_quantities, 0, sizeof(cell->material_quantities));
        }
    }
    
    return grid;
}

void grid_free(Grid* grid) {
    if (!grid) return;
    
    if (grid->cells) {
        free(grid->cells);
    }
    
    free(grid);
}

Cell* grid_get_cell(Grid* grid, int x, int y) {
    if (!grid_in_bounds(grid, x, y)) return NULL;
    
    return &(grid->cells[y * grid->width + x]);
}

bool grid_set_material(Grid* grid, int x, int y, CellMaterial material) {
    if (!grid_in_bounds(grid, x, y)) return false;
    
    Cell* cell = grid_get_cell(grid, x, y);
    cell->material = material;
    cell->density = material_properties[material].density;
    cell->temperature = material_properties[material].temperature;
    
    // Reset material quantities when changing the primary material
    memset(cell->material_quantities, 0, sizeof(cell->material_quantities));
    
    return true;
}

void grid_reset_updates(Grid* grid) {
    if (!grid) return;
    
    for (uint32_t y = 0; y < grid->height; y++) {
        for (uint32_t x = 0; x < grid->width; x++) {
            Cell* cell = grid_get_cell(grid, x, y);
            cell->updated = false;
        }
    }
    
    // Increment frame counter
    grid->frame_counter++;
}

MaterialProperties grid_get_material_properties(CellMaterial material) {
    if (material >= MATERIAL_COUNT) {
        // Return air properties for invalid materials
        return material_properties[MATERIAL_AIR];
    }
    
    return material_properties[material];
}

bool grid_in_bounds(Grid* grid, int x, int y) {
    if (!grid) return false;
    
    return (x >= 0 && x < (int)grid->width && y >= 0 && y < (int)grid->height);
}

const char* grid_material_name(CellMaterial material) {
    if (material >= MATERIAL_COUNT) {
        return "Unknown";
    }
    
    return material_names[material];
}
