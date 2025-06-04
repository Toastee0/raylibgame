/**
 * @file simulation.c
 * @brief Implementation of the simulation functions
 */

#include "simulation.h"
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

// Global simulation settings
static struct {
    float gravity;          // Gravity strength
    float viscosity_factor; // How much viscosity affects flow
    int dispersion_rate;    // How quickly materials disperse in air/water
    float temp_transfer;    // Heat transfer coefficient
    float evaporation_temp; // Temperature at which water evaporates
    float freezing_temp;    // Temperature at which water freezes
    bool paused;            // Whether simulation is paused
} sim_settings = {
    .gravity = 1.0f,
    .viscosity_factor = 0.5f,
    .dispersion_rate = 3,
    .temp_transfer = 0.2f,
    .evaporation_temp = 100.0f,
    .freezing_temp = 0.0f,
    .paused = false
};

bool simulation_init(void) {
    // Any additional setup could go here
    return true;
}

void simulation_cleanup(void) {
    // Any cleanup could go here
}

void simulation_update(Grid* grid) {
    if (!grid || sim_settings.paused) return;
    
    // Reset all cell update flags
    grid_reset_updates(grid);
    
    // Update from bottom-up for falling materials (important for sand-like behavior)
    for (int y = grid->height - 1; y >= 0; y--) {
        // Even frames go left to right, odd frames go right to left (for more natural dispersal)
        if (grid->frame_counter % 2 == 0) {
            for (int x = 0; x < (int)grid->width; x++) {
                update_cell(grid, x, y);
            }
        } else {
            for (int x = grid->width - 1; x >= 0; x--) {
                update_cell(grid, x, y);
            }
        }
    }
    
    // Temperature effects (heat transfer, phase changes)
    update_temperature(grid);
    
    // Material transport
    update_material_transport(grid);
    
    // Fire and combustion effects
    update_fire(grid);
}

void update_cell(Grid* grid, int x, int y) {
    Cell* cell = grid_get_cell(grid, x, y);
    if (!cell || cell->updated) return;
    
    bool updated = false;
      // Handle cell based on its type
    switch (cell->material) {
        case MATERIAL_SAND:
        case MATERIAL_SOIL:
            updated = update_solid(grid, x, y, cell);
            break;
            
        case MATERIAL_STONE:
            // Stone is static and doesn't move - useful for building cave systems
            break;
            
        case MATERIAL_WATER:
        case MATERIAL_OIL:
            updated = update_liquid(grid, x, y, cell);
            break;
            
        case MATERIAL_AIR:
        case MATERIAL_STEAM:
            updated = update_gas(grid, x, y, cell);
            break;
            
        case MATERIAL_FIRE:
            updated = update_fire_cell(grid, x, y, cell);
            break;
            
        case MATERIAL_WOOD:
            // Wood is static but might catch fire
            break;
            
        case MATERIAL_ICE:
            // Ice is static but might melt
            break;
            
        default:
            break;
    }
    
    if (updated) {
        cell->updated = true;
    }
}

bool update_solid(Grid* grid, int x, int y, Cell* cell) {
    // Skip if already updated or at the bottom of the grid
    if (cell->updated || y >= (int)grid->height - 1) return false;
    
    // Basic falling solid behavior
    // Check cell directly below
    Cell* below = grid_get_cell(grid, x, y + 1);
    if (below && (below->material == MATERIAL_AIR || 
                 (below->material == MATERIAL_WATER && cell->density > below->density))) {
        // Swap materials
        CellMaterial temp_mat = below->material;
        below->material = cell->material;
        below->density = cell->density;
        below->temperature = cell->temperature;
        
        cell->material = temp_mat;
        cell->density = grid_get_material_properties(temp_mat).density;
        cell->temperature = below->temperature; // Preserve temp for now, more complex handling in temperature update
        
        below->updated = true;
        return true;
    }
      // If can't fall straight down, try falling diagonally
    int dx = (rand() % 2) * 2 - 1; // -1 or 1
    Cell* diagonal = grid_get_cell(grid, x + dx, y + 1);
    if (diagonal && (diagonal->material == MATERIAL_AIR || 
                     (diagonal->material == MATERIAL_WATER && cell->density > diagonal->density))) {
        // Swap materials
        CellMaterial temp_mat = diagonal->material;
        diagonal->material = cell->material;
        diagonal->density = cell->density;
        diagonal->temperature = cell->temperature;
        
        cell->material = temp_mat;
        cell->density = grid_get_material_properties(temp_mat).density;
        cell->temperature = diagonal->temperature;
        
        diagonal->updated = true;
        return true;
    }
    
    return false;
}

bool update_liquid(Grid* grid, int x, int y, Cell* cell) {
    // Skip if already updated or at the bottom of the grid
    if (cell->updated || y >= (int)grid->height - 1) return false;
      // Try falling down first
    Cell* below = grid_get_cell(grid, x, y + 1);
    if (below && (below->material == MATERIAL_AIR || 
                 (below->material != cell->material && below->density < cell->density))) {
        // Swap materials
        CellMaterial temp_mat = below->material;
        below->material = cell->material;
        below->density = cell->density;
        below->temperature = cell->temperature;
        
        cell->material = temp_mat;
        cell->density = grid_get_material_properties(temp_mat).density;
        cell->temperature = below->temperature;
        
        below->updated = true;
        return true;
    }
    
    // Try flowing diagonally down
    int dx = (rand() % 2) * 2 - 1; // -1 or 1
    Cell* diagonal = grid_get_cell(grid, x + dx, y + 1);    if (diagonal && (diagonal->material == MATERIAL_AIR || 
                    (diagonal->material != cell->material && diagonal->density < cell->density))) {
        // Swap materials
        CellMaterial temp_mat = diagonal->material;
        diagonal->material = cell->material;
        diagonal->density = cell->density;
        diagonal->temperature = cell->temperature;
        
        cell->material = temp_mat;
        cell->density = grid_get_material_properties(temp_mat).density;
        cell->temperature = diagonal->temperature;
        
        diagonal->updated = true;
        return true;
    }
    
    // Try flowing horizontally
    int flow_direction = (rand() % 2) * 2 - 1; // -1 or 1
    Cell* side = grid_get_cell(grid, x + flow_direction, y);
    if (side && side->material == MATERIAL_AIR) {
        // Swap materials
        side->material = cell->material;
        side->density = cell->density;
        side->temperature = cell->temperature;
        
        cell->material = MATERIAL_AIR;
        cell->density = grid_get_material_properties(MATERIAL_AIR).density;
        
        side->updated = true;
        return true;
    }
    
    return false;
}

bool update_gas(Grid* grid, int x, int y, Cell* cell) {
    // Skip if already updated or at the top of the grid
    if (cell->updated || y <= 0) return false;
    
    // For gases, buoyancy causes them to rise
    if (cell->material == MATERIAL_AIR) {
        // Air is mostly static but can move if pushed
        return false;
    }
    
    // For other gases like steam, try rising up
    Cell* above = grid_get_cell(grid, x, y - 1);
    if (above && (above->material == MATERIAL_AIR || 
                 (above->material != cell->material && above->density > cell->density))) {
        // Swap materials
        CellMaterial temp_mat = above->material;
        above->material = cell->material;
        above->density = cell->density;
        above->temperature = cell->temperature;
        
        cell->material = temp_mat;
        cell->density = grid_get_material_properties(temp_mat).density;
        cell->temperature = above->temperature;
        
        above->updated = true;
        return true;
    }
    
    // Try floating diagonally up
    int dx = (rand() % 2) * 2 - 1; // -1 or 1
    Cell* diagonal = grid_get_cell(grid, x + dx, y - 1);
    if (diagonal && (diagonal->material == MATERIAL_AIR || 
                    (diagonal->material != cell->material && diagonal->density > cell->density))) {
        // Swap materials
        CellMaterial temp_mat = diagonal->material;
        diagonal->material = cell->material;
        diagonal->density = cell->density;
        diagonal->temperature = cell->temperature;
        
        cell->material = temp_mat;
        cell->density = grid_get_material_properties(temp_mat).density;
        cell->temperature = diagonal->temperature;
        
        diagonal->updated = true;
        return true;
    }
    
    // Try flowing horizontally
    int flow_direction = (rand() % 2) * 2 - 1; // -1 or 1
    Cell* side = grid_get_cell(grid, x + flow_direction, y);
    if (side && side->material == MATERIAL_AIR) {
        // Swap materials
        side->material = cell->material;
        side->density = cell->density;
        side->temperature = cell->temperature;
        
        cell->material = MATERIAL_AIR;
        cell->density = grid_get_material_properties(MATERIAL_AIR).density;
        
        side->updated = true;
        return true;
    }
    
    return false;
}

bool update_fire_cell(Grid* grid, int x, int y, Cell* cell) {
    // Fire spreads to adjacent flammable cells, converts to air over time
    
    // Fire has a chance to spread to adjacent cells
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            
            Cell* neighbor = grid_get_cell(grid, x + dx, y + dy);
            if (neighbor && grid_get_material_properties(neighbor->material).flammable && 
                rand() % 100 < 5) { // 5% chance to ignite
                neighbor->material = MATERIAL_FIRE;
                neighbor->temperature = grid_get_material_properties(MATERIAL_FIRE).temperature;
                neighbor->updated = true;
            }
        }
    }
    
    // Fire burns out over time
    if (rand() % 100 < 10) { // 10% chance to burn out each frame
        cell->material = MATERIAL_AIR;
        cell->density = grid_get_material_properties(MATERIAL_AIR).density;
        cell->temperature = 50.0f; // Hot air
        return true;
    }
    
    return false;
}

void simulation_add_material(Grid* grid, int x, int y, int radius, CellMaterial material) {
    if (!grid) return;
    
    // Add material in a circle
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            // Skip if outside the circle
            if (dx*dx + dy*dy > radius*radius) continue;
            
            grid_set_material(grid, x + dx, y + dy, material);
        }
    }
}

void update_temperature(Grid* grid) {
    if (!grid) return;
    
    // Simple heat transfer between adjacent cells
    for (uint32_t y = 0; y < grid->height; y++) {
        for (uint32_t x = 0; x < grid->width; x++) {
            Cell* cell = grid_get_cell(grid, x, y);
            if (!cell) continue;
            
            // Calculate average temperature of neighbors
            float sum_temp = 0.0f;
            int count = 0;
            
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    
                    Cell* neighbor = grid_get_cell(grid, x + dx, y + dy);
                    if (neighbor) {
                        sum_temp += neighbor->temperature;
                        count++;
                    }
                }
            }
            
            if (count > 0) {
                float avg_temp = sum_temp / count;
                float conductivity = grid_get_material_properties(cell->material).heat_conductivity;
                
                // Adjust cell temperature based on neighbors and conductivity
                cell->temperature += (avg_temp - cell->temperature) * conductivity * sim_settings.temp_transfer;
            }
            
            // Phase changes
            if (cell->material == MATERIAL_WATER) {
                // Water to steam
                if (cell->temperature >= sim_settings.evaporation_temp && rand() % 100 < 5) {
                    cell->material = MATERIAL_STEAM;
                    cell->density = grid_get_material_properties(MATERIAL_STEAM).density;
                }
                // Water to ice
                else if (cell->temperature <= sim_settings.freezing_temp && rand() % 100 < 5) {
                    cell->material = MATERIAL_ICE;
                    cell->density = grid_get_material_properties(MATERIAL_ICE).density;
                }
            } 
            // Steam to water (condensation)
            else if (cell->material == MATERIAL_STEAM) {
                if (cell->temperature < 100.0f && rand() % 100 < 1) {
                    cell->material = MATERIAL_WATER;
                    cell->density = grid_get_material_properties(MATERIAL_WATER).density;
                }
            }
            // Ice to water (melting)
            else if (cell->material == MATERIAL_ICE) {
                if (cell->temperature > 0.0f && rand() % 100 < 5) {
                    cell->material = MATERIAL_WATER;
                    cell->density = grid_get_material_properties(MATERIAL_WATER).density;
                }
            }
        }
    }
}

void update_material_transport(Grid* grid) {
    if (!grid) return;
    
    // Simplified material transport - fluids can carry other materials
    for (uint32_t y = 0; y < grid->height; y++) {
        for (uint32_t x = 0; x < grid->width; x++) {
            Cell* cell = grid_get_cell(grid, x, y);
            if (!cell || !grid_get_material_properties(cell->material).can_carry_materials) continue;
            
            // For each neighbor that has moved this frame
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    
                    Cell* neighbor = grid_get_cell(grid, x + dx, y + dy);
                    if (!neighbor || !neighbor->updated) continue;
                    
                    // Transfer a small amount of carried materials between cells
                    // This would need a more sophisticated implementation for a real simulation
                    for (int mat = 0; mat < MATERIAL_COUNT; mat++) {
                        if (cell->material_quantities[mat] > 0 && rand() % 100 < sim_settings.dispersion_rate) {
                            // Transfer some material from cell to neighbor
                            uint8_t transfer_amount = cell->material_quantities[mat] / 10;
                            if (transfer_amount > 0) {
                                cell->material_quantities[mat] -= transfer_amount;
                                neighbor->material_quantities[mat] += transfer_amount;
                            }
                        }
                    }
                }
            }
        }
    }
}

void update_fire(Grid* grid) {
    if (!grid) return;
    
    // Fire simulation - check for combustible materials and high temperatures
    for (uint32_t y = 0; y < grid->height; y++) {
        for (uint32_t x = 0; x < grid->width; x++) {
            Cell* cell = grid_get_cell(grid, x, y);
            if (!cell) continue;
            
            // If the cell is flammable and hot enough, it might catch fire
            if (grid_get_material_properties(cell->material).flammable && 
                cell->temperature > 200.0f && rand() % 100 < 1) { // 1% chance if hot enough
                cell->material = MATERIAL_FIRE;
                cell->temperature = grid_get_material_properties(MATERIAL_FIRE).temperature;
            }
        }
    }
}
