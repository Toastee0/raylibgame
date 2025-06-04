/**
 * @file debug_utils.c
 * @brief Implementation of debugging utility functions
 */

#include "debug_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Material name strings for JSON output
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

char get_material_char(CellMaterial material) {
    switch (material) {
        case MATERIAL_AIR:    return ' ';
        case MATERIAL_SAND:   return 'S';
        case MATERIAL_WATER:  return 'W';
        case MATERIAL_SOIL:   return 'E'; // Earth/soil
        case MATERIAL_STONE:  return 'R'; // Rock/stone
        case MATERIAL_WOOD:   return 'T'; // Tree/wood
        case MATERIAL_FIRE:   return 'F';
        case MATERIAL_STEAM:  return 'V'; // Vapor/steam
        case MATERIAL_ICE:    return 'I';
        case MATERIAL_OIL:    return 'O';
        default:              return '?';
    }
}

void print_grid_to_stream(Grid* grid, FILE* stream, int region_x, int region_y, int width, int height) {
    if (!grid || !stream) return;
    
    // Clamp the region to grid bounds
    if (region_x < 0) region_x = 0;
    if (region_y < 0) region_y = 0;
    if (region_x + width > (int)grid->width) width = grid->width - region_x;
    if (region_y + height > (int)grid->height) height = grid->height - region_y;
    
    // Print grid header
    fprintf(stream, "Grid State (Frame %u) - Region [%d,%d] to [%d,%d]:\n", 
            grid->frame_counter, region_x, region_y, region_x + width - 1, region_y + height - 1);
    
    // Print column numbers
    fprintf(stream, "   ");
    for (int x = 0; x < width; x++) {
        fprintf(stream, "%d", (region_x + x) % 10);
    }
    fprintf(stream, "\n");
    
    // Print the grid
    for (int y = 0; y < height; y++) {
        fprintf(stream, "%2d|", region_y + y);
        
        for (int x = 0; x < width; x++) {
            Cell* cell = grid_get_cell(grid, region_x + x, region_y + y);
            if (cell) {
                fprintf(stream, "%c", get_material_char(cell->material));
            } else {
                fprintf(stream, "?");
            }
        }
        
        fprintf(stream, "|\n");
    }
}

void print_grid_console(Grid* grid, int region_x, int region_y, int width, int height) {
    print_grid_to_stream(grid, stdout, region_x, region_y, width, height);
}

bool save_grid_to_file(Grid* grid, const char* filename) {
    if (!grid || !filename) return false;
    
    FILE* file = fopen(filename, "w");
    if (!file) return false;
    
    // Write JSON header with metadata
    fprintf(file, "{\n");
    fprintf(file, "  \"grid_width\": %u,\n", grid->width);
    fprintf(file, "  \"grid_height\": %u,\n", grid->height);
    fprintf(file, "  \"frame\": %u,\n", grid->frame_counter);
    
    // Write material map
    fprintf(file, "  \"material_map\": {\n");
    for (int i = 0; i < MATERIAL_COUNT; i++) {
        fprintf(file, "    \"%d\": \"%s\"%s\n", i, material_names[i], 
                (i < MATERIAL_COUNT - 1) ? "," : "");
    }
    fprintf(file, "  },\n");
    
    // Start writing cells array
    fprintf(file, "  \"cells\": [\n");
    
    // Write each cell
    for (uint32_t y = 0; y < grid->height; y++) {
        for (uint32_t x = 0; x < grid->width; x++) {
            Cell* cell = grid_get_cell(grid, x, y);
            
            if (!cell) continue;
            
            // For large grids, only output non-air cells to reduce file size
            if (grid->width * grid->height > 10000 && cell->material == MATERIAL_AIR) {
                continue;
            }
            
            fprintf(file, "    {\n");
            fprintf(file, "      \"x\": %u,\n", x);
            fprintf(file, "      \"y\": %u,\n", y);
            fprintf(file, "      \"material\": %d,\n", cell->material);
            fprintf(file, "      \"material_name\": \"%s\",\n", material_names[cell->material]);
            fprintf(file, "      \"density\": %.2f,\n", cell->density);
            fprintf(file, "      \"temperature\": %.2f,\n", cell->temperature);
            fprintf(file, "      \"pressure\": %.2f,\n", cell->pressure);
            
            // Only include material quantities if any are non-zero
            bool has_quantities = false;
            for (int i = 0; i < MATERIAL_COUNT; i++) {
                if (cell->material_quantities[i] > 0) {
                    has_quantities = true;
                    break;
                }
            }
            
            if (has_quantities) {
                fprintf(file, "      \"material_quantities\": {\n");
                bool first = true;
                for (int i = 0; i < MATERIAL_COUNT; i++) {
                    if (cell->material_quantities[i] > 0) {
                        if (!first) fprintf(file, ",\n");
                        fprintf(file, "        \"%s\": %u", material_names[i], cell->material_quantities[i]);
                        first = false;
                    }
                }
                fprintf(file, "\n      },\n");
            } else {
                fprintf(file, "      \"material_quantities\": {},\n");
            }
            
            fprintf(file, "      \"updated\": %s\n", cell->updated ? "true" : "false");
            
            // Check if this is the last cell to write
            bool is_last_cell = (y == grid->height - 1 && x == grid->width - 1);
            // For large grids, we need to scan ahead to find the last non-air cell
            if (grid->width * grid->height > 10000) {
                is_last_cell = true;
                bool found_more = false;
                
                // Scan ahead for more non-air cells
                for (uint32_t scan_y = y; scan_y < grid->height; scan_y++) {
                    uint32_t scan_start_x = (scan_y == y) ? x + 1 : 0;
                    for (uint32_t scan_x = scan_start_x; scan_x < grid->width; scan_x++) {
                        Cell* scan_cell = grid_get_cell(grid, scan_x, scan_y);
                        if (scan_cell && scan_cell->material != MATERIAL_AIR) {
                            found_more = true;
                            break;
                        }
                    }
                    if (found_more) break;
                }
                
                is_last_cell = !found_more;
            }
            
            fprintf(file, "    }%s\n", is_last_cell ? "" : ",");
        }
    }
    
    fprintf(file, "  ]\n");
    fprintf(file, "}\n");
    
    fclose(file);
    return true;
}

bool save_grid_region_to_file(Grid* grid, const char* filename, int region_x, int region_y, int width, int height) {
    if (!grid || !filename) return false;
    
    // Clamp the region to grid bounds
    if (region_x < 0) region_x = 0;
    if (region_y < 0) region_y = 0;
    if (region_x + width > (int)grid->width) width = grid->width - region_x;
    if (region_y + height > (int)grid->height) height = grid->height - region_y;
    
    FILE* file = fopen(filename, "w");
    if (!file) return false;
    
    // Write JSON header with metadata
    fprintf(file, "{\n");
    fprintf(file, "  \"region_x\": %d,\n", region_x);
    fprintf(file, "  \"region_y\": %d,\n", region_y);
    fprintf(file, "  \"region_width\": %d,\n", width);
    fprintf(file, "  \"region_height\": %d,\n", height);
    fprintf(file, "  \"frame\": %u,\n", grid->frame_counter);
    
    // Write material map
    fprintf(file, "  \"material_map\": {\n");
    for (int i = 0; i < MATERIAL_COUNT; i++) {
        fprintf(file, "    \"%d\": \"%s\"%s\n", i, material_names[i], 
                (i < MATERIAL_COUNT - 1) ? "," : "");
    }
    fprintf(file, "  },\n");
    
    // Start writing cells array
    fprintf(file, "  \"cells\": [\n");
    
    // Write cells in the specified region
    bool first_cell = true;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            Cell* cell = grid_get_cell(grid, region_x + x, region_y + y);
            if (!cell) continue;
            
            if (!first_cell) fprintf(file, ",\n");
            first_cell = false;
            
            fprintf(file, "    {\n");
            fprintf(file, "      \"local_x\": %d,\n", x);
            fprintf(file, "      \"local_y\": %d,\n", y);
            fprintf(file, "      \"global_x\": %d,\n", region_x + x);
            fprintf(file, "      \"global_y\": %d,\n", region_y + y);
            fprintf(file, "      \"material\": %d,\n", cell->material);
            fprintf(file, "      \"material_name\": \"%s\",\n", material_names[cell->material]);
            fprintf(file, "      \"density\": %.2f,\n", cell->density);
            fprintf(file, "      \"temperature\": %.2f,\n", cell->temperature);
            fprintf(file, "      \"pressure\": %.2f,\n", cell->pressure);
            
            // Include material quantities
            fprintf(file, "      \"material_quantities\": {\n");
            bool first_quantity = true;
            for (int i = 0; i < MATERIAL_COUNT; i++) {
                if (cell->material_quantities[i] > 0) {
                    if (!first_quantity) fprintf(file, ",\n");
                    first_quantity = false;
                    fprintf(file, "        \"%s\": %u", material_names[i], cell->material_quantities[i]);
                }
            }
            fprintf(file, "\n      },\n");
            
            fprintf(file, "      \"updated\": %s\n", cell->updated ? "true" : "false");
            fprintf(file, "    }");
        }
    }
    
    fprintf(file, "\n  ]\n");
    fprintf(file, "}\n");
    
    fclose(file);
    return true;
}

bool save_cell_details_to_file(Grid* grid, const char* filename, int x, int y) {
    if (!grid || !filename) return false;
    
    Cell* cell = grid_get_cell(grid, x, y);
    if (!cell) return false;
    
    FILE* file = fopen(filename, "w");
    if (!file) return false;
    
    // Write cell details as JSON
    fprintf(file, "{\n");
    fprintf(file, "  \"frame\": %u,\n", grid->frame_counter);
    fprintf(file, "  \"cell\": {\n");
    fprintf(file, "    \"x\": %d,\n", x);
    fprintf(file, "    \"y\": %d,\n", y);
    fprintf(file, "    \"material\": %d,\n", cell->material);
    fprintf(file, "    \"material_name\": \"%s\",\n", material_names[cell->material]);
    fprintf(file, "    \"density\": %.2f,\n", cell->density);
    fprintf(file, "    \"temperature\": %.2f,\n", cell->temperature);
    fprintf(file, "    \"pressure\": %.2f,\n", cell->pressure);
    fprintf(file, "    \"updated\": %s,\n", cell->updated ? "true" : "false");
    
    // Material quantities
    fprintf(file, "    \"material_quantities\": {\n");
    bool first_quantity = true;
    for (int i = 0; i < MATERIAL_COUNT; i++) {
        if (cell->material_quantities[i] > 0) {
            if (!first_quantity) fprintf(file, ",\n");
            first_quantity = false;
            fprintf(file, "      \"%s\": %u", material_names[i], cell->material_quantities[i]);
        }
    }
    fprintf(file, "\n    },\n");
    
    // Get surrounding cells for context
    fprintf(file, "    \"neighbors\": {\n");
    
    // Define neighbor offsets for all 8 directions
    const int neighbor_offsets[][2] = {
        {0, -1},  // Top
        {1, -1},  // Top-right
        {1, 0},   // Right
        {1, 1},   // Bottom-right
        {0, 1},   // Bottom
        {-1, 1},  // Bottom-left
        {-1, 0},  // Left
        {-1, -1}  // Top-left
    };
    const char* neighbor_names[] = {
        "top", "topRight", "right", "bottomRight", 
        "bottom", "bottomLeft", "left", "topLeft"
    };
    
    for (int i = 0; i < 8; i++) {
        int nx = x + neighbor_offsets[i][0];
        int ny = y + neighbor_offsets[i][1];
        Cell* neighbor_cell = grid_get_cell(grid, nx, ny);
        
        fprintf(file, "      \"%s\": ", neighbor_names[i]);
        
        if (neighbor_cell) {
            fprintf(file, "{\n");
            fprintf(file, "        \"x\": %d,\n", nx);
            fprintf(file, "        \"y\": %d,\n", ny);
            fprintf(file, "        \"material\": %d,\n", neighbor_cell->material);
            fprintf(file, "        \"material_name\": \"%s\",\n", material_names[neighbor_cell->material]);
            fprintf(file, "        \"density\": %.2f,\n", neighbor_cell->density);
            fprintf(file, "        \"temperature\": %.2f\n", neighbor_cell->temperature);
            fprintf(file, "      }");
        } else {
            fprintf(file, "null");
        }
        
        if (i < 7) fprintf(file, ",\n");
        else fprintf(file, "\n");
    }
    
    fprintf(file, "    }\n");
    fprintf(file, "  }\n");
    fprintf(file, "}\n");
    
    fclose(file);
    return true;
}

bool save_simulation_stats_to_file(Grid* grid, const char* filename) {
    if (!grid || !filename) return false;
    
    FILE* file = fopen(filename, "w");
    if (!file) return false;
    
    // Count materials
    int material_counts[MATERIAL_COUNT] = {0};
    float avg_temp = 0.0f;
    float max_temp = -1000.0f;
    float min_temp = 1000.0f;
    float avg_pressure = 0.0f;
    float max_pressure = -1000.0f;
    float min_pressure = 1000.0f;
    int total_cells = 0;
    
    for (uint32_t y = 0; y < grid->height; y++) {
        for (uint32_t x = 0; x < grid->width; x++) {
            Cell* cell = grid_get_cell(grid, x, y);
            if (!cell) continue;
            
            material_counts[cell->material]++;
            
            avg_temp += cell->temperature;
            if (cell->temperature > max_temp) max_temp = cell->temperature;
            if (cell->temperature < min_temp) min_temp = cell->temperature;
            
            avg_pressure += cell->pressure;
            if (cell->pressure > max_pressure) max_pressure = cell->pressure;
            if (cell->pressure < min_pressure) min_pressure = cell->pressure;
            
            total_cells++;
        }
    }
    
    // Calculate averages
    if (total_cells > 0) {
        avg_temp /= total_cells;
        avg_pressure /= total_cells;
    }
    
    // Write statistics as JSON
    fprintf(file, "{\n");
    fprintf(file, "  \"frame\": %u,\n", grid->frame_counter);
    fprintf(file, "  \"total_cells\": %d,\n", total_cells);
    fprintf(file, "  \"grid_dimensions\": {\n");
    fprintf(file, "    \"width\": %u,\n", grid->width);
    fprintf(file, "    \"height\": %u\n", grid->height);
    fprintf(file, "  },\n");
    
    fprintf(file, "  \"material_distribution\": {\n");
    for (int i = 0; i < MATERIAL_COUNT; i++) {
        fprintf(file, "    \"%s\": %d%s\n", material_names[i], material_counts[i],
                (i < MATERIAL_COUNT - 1) ? "," : "");
    }
    fprintf(file, "  },\n");
    
    fprintf(file, "  \"temperature\": {\n");
    fprintf(file, "    \"average\": %.2f,\n", avg_temp);
    fprintf(file, "    \"max\": %.2f,\n", max_temp);
    fprintf(file, "    \"min\": %.2f\n", min_temp);
    fprintf(file, "  },\n");
    
    fprintf(file, "  \"pressure\": {\n");
    fprintf(file, "    \"average\": %.2f,\n", avg_pressure);
    fprintf(file, "    \"max\": %.2f,\n", max_pressure);
    fprintf(file, "    \"min\": %.2f\n", min_pressure);
    fprintf(file, "  }\n");
    
    fprintf(file, "}\n");
    
    fclose(file);
    return true;
}
