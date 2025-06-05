/**
 * @file rendering.c
 * @brief Implementation of the rendering functions
 */

#include "rendering.h"
#include "color_utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// Rendering settings
static struct {
    int cell_size;     // Size of a cell in pixels
    int screen_width;  // Screen width in pixels
    int screen_height; // Screen height in pixels
    float zoom;        // Zoom level
    float pan_x;       // X pan offset
    float pan_y;       // Y pan offset
    Texture2D cell_textures[MATERIAL_COUNT]; // Textures for each material type
    bool use_textures; // Whether to use textures or simple colors
    Font debug_font;   // Font for debug text
    float dpi_scale;   // DPI scaling factor
} render_settings = {
    .cell_size = 8,     // Default cell size (increased to 8x8)
    .zoom = 1.0f,       // Default zoom level
    .pan_x = 0.0f,      // Default pan offset
    .pan_y = 0.0f,      // Default pan offset
    .use_textures = false, // Use simple colors by default
    .dpi_scale = 1.0f   // Default DPI scale
};

bool rendering_init(int screen_width, int screen_height) {
    render_settings.screen_width = screen_width;
    render_settings.screen_height = screen_height;
    
    // Get DPI scaling from Raylib
    render_settings.dpi_scale = GetWindowScaleDPI().x;  // Use X scale (usually X and Y are the same)
    TraceLog(LOG_INFO, "DPI Scale detected: %.2f", render_settings.dpi_scale);
    
    // Start with base cell size of 8 pixels (will be adjusted later based on window size)
    render_settings.cell_size = (int)(8 * render_settings.dpi_scale);
    
    // Set initial pan to top-left (just below the UI bar)
    render_settings.pan_x = 0;
    int button_size = (int)(30 * render_settings.dpi_scale);
    int padding = (int)(5 * render_settings.dpi_scale);
    render_settings.pan_y = button_size + padding * 2; // UI bar height
    
    // Note: We'll calculate the optimal cell size later once the grid is initialized
    
    // Initialize textures if needed
    if (render_settings.use_textures) {
        // Load textures for materials
        for (int i = 0; i < MATERIAL_COUNT; i++) {
            Image img = GenImageColor(16, 16, ColorFromInt(grid_get_material_properties(i).color));
            render_settings.cell_textures[i] = LoadTextureFromImage(img);
            UnloadImage(img);
        }
    }
    
    // Load debug font
    render_settings.debug_font = GetFontDefault();
    
    return true;
}

void rendering_cleanup(void) {
    // Unload textures if they were loaded
    if (render_settings.use_textures) {
        for (int i = 0; i < MATERIAL_COUNT; i++) {
            UnloadTexture(render_settings.cell_textures[i]);
        }
    }
}

void rendering_draw_grid(Grid* grid) {
    if (!grid) return;
    
    // Calculate UI height to avoid drawing under it
    float dpi_scale = render_settings.dpi_scale;
    int button_size = (int)(30 * dpi_scale);
    int padding = (int)(5 * dpi_scale);
    int top_bar_height = button_size + padding * 2;
    
    // Calculate the portion of the grid that's visible
    int start_x = (int)fmaxf(0, -render_settings.pan_x / (render_settings.cell_size * render_settings.zoom));
    int start_y = (int)fmaxf(0, -render_settings.pan_y / (render_settings.cell_size * render_settings.zoom));
    int end_x = (int)fminf(grid->width, start_x + render_settings.screen_width / (render_settings.cell_size * render_settings.zoom) + 1);
    int end_y = (int)fminf(grid->height, start_y + (render_settings.screen_height - top_bar_height) / (render_settings.cell_size * render_settings.zoom) + 1);
    
    // Draw each visible cell
    for (int y = start_y; y < end_y; y++) {
        for (int x = start_x; x < end_x; x++) {            Cell* cell = grid_get_cell(grid, x, y);
            if (!cell) continue;
            
            // Handle air cells - only draw if they contain humidity (for cloud effects)
            if (cell->material == MATERIAL_AIR) {
                // Check for humidity (MATERIAL_WATER carried in air)
                if (cell->material_quantities[MATERIAL_WATER] > 0) {
                    // Calculate humidity level for cloud effect (white to dark gray based on moisture)
                    uint8_t humidity = cell->material_quantities[MATERIAL_WATER];
                    
                    // Only display clouds when humidity is above 75% of capacity
                    // (humidity is 0-255, so 192 is ~75%)
                    if (humidity > 192) {
                        // Calculate cloud color (255 = white, 128 = gray for darker clouds)
                        uint8_t cloud_value = 255 - (uint8_t)((humidity - 192) / 63.0f * 127); 
                        
                        // Calculate screen position
                        int screen_x, screen_y;
                        rendering_grid_to_screen(x, y, &screen_x, &screen_y);
                        
                        // Draw the cloud particle
                        DrawRectangle(
                            screen_x, 
                            screen_y, 
                            (int)(render_settings.cell_size * render_settings.zoom), 
                            (int)(render_settings.cell_size * render_settings.zoom), 
                            (Color){ cloud_value, cloud_value, cloud_value, humidity }
                        );
                    }
                }
                continue; // Skip other processing for air
            }
            
            // Calculate screen position
            int screen_x, screen_y;
            rendering_grid_to_screen(x, y, &screen_x, &screen_y);
            
            // Get cell color based on its properties
            Color color = rendering_get_cell_color(cell);
            
            // Draw the cell
            if (render_settings.use_textures) {
                // Use texture
                DrawTextureEx(render_settings.cell_textures[cell->material], 
                             (Vector2){ (float)screen_x, (float)screen_y }, 
                             0.0f, 
                             render_settings.cell_size * render_settings.zoom / 16.0f, 
                             color);
            } else {
                // Use simple rectangle with color
                DrawRectangle(
                    screen_x, 
                    screen_y, 
                    (int)(render_settings.cell_size * render_settings.zoom), 
                    (int)(render_settings.cell_size * render_settings.zoom), 
                    color
                );
            }
        }
    }
}

void rendering_draw_ui(Grid* grid, CellMaterial selected_material, int brush_size, bool paused, bool show_debug) {
    // Draw a material selection bar at the top
    float dpi_scale = render_settings.dpi_scale;
    int button_size = (int)(30 * dpi_scale);
    int padding = (int)(5 * dpi_scale);
    int top_bar_height = button_size + padding * 2;
    
    // Draw top panel background
    DrawRectangle(0, 0, render_settings.screen_width, top_bar_height, ColorAlpha(RAYWHITE, 0.8f));
    
    // Draw material selection buttons
    int x_pos = padding;
    for (int i = 0; i < MATERIAL_COUNT; i++) {
        Color btn_color = ColorFromInt(grid_get_material_properties(i).color);
        Color outline_color = (selected_material == i) ? RED : GRAY;
        
        // Draw button
        DrawRectangle(x_pos, padding, button_size, button_size, btn_color);
        DrawRectangleLines(x_pos, padding, button_size, button_size, outline_color);
        
        // Draw material name
        const char* mat_name = grid_material_name(i);
        int font_size = (int)(10 * dpi_scale);
        DrawText(mat_name, x_pos + button_size/2 - MeasureText(mat_name, font_size)/2, 
                 padding + button_size + 2, font_size, BLACK);
        
        x_pos += button_size + padding;
    }
    
    // Draw brush size indicator
    char brush_text[32];
    sprintf(brush_text, "Brush: %d", brush_size);
    int brush_font_size = (int)(20 * dpi_scale);
    int brush_width = MeasureText(brush_text, brush_font_size);
    DrawText(brush_text, x_pos, padding + 5, brush_font_size, BLACK);
    x_pos += brush_width + padding * 2;
    
    // Draw pause/play button
    const char* pause_text = paused ? "PLAY" : "PAUSE";
    int button_font_size = (int)(20 * dpi_scale);
    int pause_width = MeasureText(pause_text, button_font_size);
    
    DrawRectangle(x_pos, padding, pause_width + padding * 2, button_size, paused ? GREEN : ORANGE);
    DrawText(pause_text, x_pos + padding, padding + 5, button_font_size, BLACK);
    x_pos += pause_width + padding * 3;
    
    // Draw debug toggle button
    const char* debug_text = show_debug ? "DEBUG: ON" : "DEBUG: OFF";
    int debug_width = MeasureText(debug_text, button_font_size);
    
    DrawRectangle(x_pos, padding, debug_width + padding * 2, button_size, show_debug ? BLUE : DARKGRAY);
    DrawText(debug_text, x_pos + padding, padding + 5, button_font_size, WHITE);
    
    // Draw fps counter in corner
    DrawFPS(render_settings.screen_width - (int)(100 * dpi_scale), (int)(10 * dpi_scale));
    
    // Draw simulation stats
    char stats_text[128];
    sprintf(stats_text, "Frame: %u", grid ? grid->frame_counter : 0);
    int stats_font_size = (int)(20 * dpi_scale);
    DrawText(stats_text, render_settings.screen_width - (int)(150 * dpi_scale), (int)(30 * dpi_scale), stats_font_size, BLACK);
    
    // Show mouse position and what's under the cursor
    int mouse_x = GetMouseX();
    int mouse_y = GetMouseY();
    int grid_x, grid_y;
    rendering_screen_to_grid(mouse_x, mouse_y, &grid_x, &grid_y);
    
    if (grid_in_bounds(grid, grid_x, grid_y)) {
        Cell* cell = grid_get_cell(grid, grid_x, grid_y);
        if (cell) {
            char mouse_info[128];
            sprintf(mouse_info, "Pos: (%d,%d) | %s T:%.1fC", 
                    grid_x, grid_y, grid_material_name(cell->material), cell->temperature);
            int info_font_size = (int)(20 * dpi_scale);
            DrawText(mouse_info, (int)(10 * dpi_scale), render_settings.screen_height - (int)(30 * dpi_scale), info_font_size, BLACK);
            
            // If debug mode is on, show detailed info about the cell under cursor
            if (show_debug) {
                rendering_draw_cell_debug(grid, grid_x, grid_y);
            }
        }
    }
}

void rendering_grid_to_screen(int grid_x, int grid_y, int *screen_x, int *screen_y) {
    *screen_x = (int)(grid_x * render_settings.cell_size * render_settings.zoom + render_settings.pan_x);
    *screen_y = (int)(grid_y * render_settings.cell_size * render_settings.zoom + render_settings.pan_y);
    
    // No need to clip coordinates here - the drawing functions will handle clipping
    // and we want to maintain the correct mathematical relationship between grid and screen
}

void rendering_screen_to_grid(int screen_x, int screen_y, int *grid_x, int *grid_y) {
    *grid_x = (int)((screen_x - render_settings.pan_x) / (render_settings.cell_size * render_settings.zoom));
    *grid_y = (int)((screen_y - render_settings.pan_y) / (render_settings.cell_size * render_settings.zoom));
    
    // Ensure we're within grid bounds
    // Actual bounds checking will be done in grid_in_bounds when needed
}

void rendering_set_zoom(float zoom) {
    // Limit zoom range
    if (zoom < 0.25f) zoom = 0.25f;
    if (zoom > 5.0f) zoom = 5.0f;
    
    render_settings.zoom = zoom;
}

void rendering_set_pan(float pan_x, float pan_y) {
    render_settings.pan_x = pan_x;
    render_settings.pan_y = pan_y;
}

float rendering_get_dpi_scale(void) {
    return render_settings.dpi_scale;
}

int rendering_get_cell_size(void) {
    return render_settings.cell_size;
}

void rendering_calculate_optimal_cell_size(Grid* grid) {
    if (!grid) return;
    
    // Get DPI scaled values
    float dpi_scale = render_settings.dpi_scale;
    
    // Calculate UI height
    int button_size = (int)(30 * dpi_scale);
    int padding = (int)(5 * dpi_scale);
    int top_bar_height = button_size + padding * 2;
    
    // Calculate available space for grid rendering
    int available_width = render_settings.screen_width;
    int available_height = render_settings.screen_height - top_bar_height;
    
    // Calculate the maximum cell size that fits within the available space
    int max_width_based_size = available_width / grid->width;
    int max_height_based_size = available_height / grid->height;
    int max_possible_size = max_width_based_size < max_height_based_size ? max_width_based_size : max_height_based_size;
    
    // Base size is 8 pixels (scaled by DPI)
    int base_size = (int)(8 * dpi_scale);
    
    // Find the largest power of 2 multiplier that fits within the max size
    int optimal_size = base_size;
    for (int size = base_size * 2; size <= max_possible_size; size *= 2) {
        optimal_size = size;
    }
    
    // Update cell size
    render_settings.cell_size = optimal_size;
    
    // Reset zoom when recalculating
    render_settings.zoom = 1.0f;
    
    // Calculate the total grid size in pixels
    int total_grid_width = grid->width * optimal_size;
    int total_grid_height = grid->height * optimal_size;
    
    // Position the grid properly - ensure it's visible from the start
    // Always position the grid just below the UI bar
    render_settings.pan_x = 0;
    render_settings.pan_y = top_bar_height;
    
    // Check if grid would extend beyond screen edges and adjust if needed
    if (total_grid_width > available_width || total_grid_height > available_height) {
        // Grid is larger than screen - keep it at top-left but ensure origin is visible
        TraceLog(LOG_INFO, "Grid larger than screen - positioning at top-left corner");
    } else {
        // Grid is smaller than screen - center it horizontally
        render_settings.pan_x = (available_width - total_grid_width) / 2;
        TraceLog(LOG_INFO, "Grid centered horizontally at x: %d", render_settings.pan_x);
    }
    
    TraceLog(LOG_INFO, "Calculated optimal cell size: %d pixels", optimal_size);
    TraceLog(LOG_INFO, "Grid positioned at (%d,%d), cell size: %d", 
             render_settings.pan_x, render_settings.pan_y, optimal_size);
}

void rendering_handle_window_resize(void) {
    // Update screen dimensions
    render_settings.screen_width = GetScreenWidth();
    render_settings.screen_height = GetScreenHeight();
    
    // Update DPI scale value in case it changed (e.g., moving window to new monitor)
    render_settings.dpi_scale = GetWindowScaleDPI().x;
    
    // Note: We don't call rendering_calculate_optimal_cell_size here because
    // that's handled in main.c after this function is called
    
    // We keep the current pan and zoom settings - they will be properly
    // recalculated in main.c when rendering_calculate_optimal_cell_size is called
    
    TraceLog(LOG_INFO, "Window resized to %dx%d, DPI scale: %.2f", 
             render_settings.screen_width, render_settings.screen_height, render_settings.dpi_scale);
}

Color rendering_get_cell_color(Cell* cell) {
    if (!cell) return BLACK;
    
    // Get base color for material
    uint32_t base_color = grid_get_material_properties(cell->material).color;
    Color color = ColorFromInt(base_color);
    
    // Modify color based on cell properties
    
    // Temperature affects color (hotter = more red/yellow, colder = more blue)
    if (cell->temperature > 100.0f) {
        // Hot materials become more red/yellow
        float temp_factor = fminf((cell->temperature - 100.0f) / 900.0f, 1.0f); // Normalize 100-1000 degrees
        color.r = (unsigned char)fminf(color.r + temp_factor * (255 - color.r), 255);
        color.g = (unsigned char)fminf(color.g + temp_factor * (200 - color.g) * 0.7f, 255);
    } else if (cell->temperature < 0.0f) {
        // Cold materials become more blue
        float temp_factor = fminf(fabsf(cell->temperature) / 100.0f, 1.0f); // Normalize 0 to -100 degrees
        color.b = (unsigned char)fminf(color.b + temp_factor * (255 - color.b), 255);
        color.r = (unsigned char)(color.r * (1.0f - temp_factor * 0.5f));
        color.g = (unsigned char)(color.g * (1.0f - temp_factor * 0.3f));
    }
    
    // Pressure could affect opacity or other aspects
    if (cell->pressure > 0.0f) {
        // Higher pressure makes cells slightly more opaque
        float pressure_factor = fminf(cell->pressure / 10.0f, 1.0f);
        color.a = (unsigned char)fminf(color.a + pressure_factor * (255 - color.a), 255);
    }
    
    // Add subtle variations to make it look more natural
    if (cell->material != MATERIAL_AIR) {
        // Add some noise to the color components for natural variation
        int noise = rand() % 20 - 10; // -10 to +10
        color.r = (unsigned char)fminf(fmaxf(color.r + noise, 0), 255);
        color.g = (unsigned char)fminf(fmaxf(color.g + noise, 0), 255);
        color.b = (unsigned char)fminf(fmaxf(color.b + noise, 0), 255);
    }
    
    // If it's a fluid like water or air and it's carrying other materials, blend the colors
    if (grid_get_material_properties(cell->material).can_carry_materials) {
        for (int i = 0; i < MATERIAL_COUNT; i++) {
            if (cell->material_quantities[i] > 0) {
                // Blend with carried material color based on quantity
                float blend_factor = cell->material_quantities[i] / 255.0f * 0.7f; // Max 70% influence
                Color carried_color = ColorFromInt(grid_get_material_properties(i).color);
                color.r = (unsigned char)((1.0f - blend_factor) * color.r + blend_factor * carried_color.r);
                color.g = (unsigned char)((1.0f - blend_factor) * color.g + blend_factor * carried_color.g);
                color.b = (unsigned char)((1.0f - blend_factor) * color.b + blend_factor * carried_color.b);
            }
        }
    }
    
    return color;
}

void rendering_draw_cell_debug(Grid* grid, int x, int y) {
    Cell* cell = grid_get_cell(grid, x, y);
    if (!cell) return;
    
    // Apply DPI scaling
    float dpi_scale = render_settings.dpi_scale;
    
    // Create debug panel at the bottom right
    int panel_width = (int)(300 * dpi_scale);
    int panel_height = (int)(200 * dpi_scale);
    int panel_x = render_settings.screen_width - panel_width - (int)(10 * dpi_scale);
    int panel_y = render_settings.screen_height - panel_height - (int)(10 * dpi_scale);
    
    // Draw panel background
    DrawRectangle(panel_x, panel_y, panel_width, panel_height, ColorAlpha(LIGHTGRAY, 0.9f));
    DrawRectangleLines(panel_x, panel_y, panel_width, panel_height, DARKGRAY);
    
    // Draw cell information
    char info[512];
    int text_y = panel_y + (int)(10 * dpi_scale);
    
    // Cell coordinates and type
    sprintf(info, "Cell (%d,%d): %s", x, y, grid_material_name(cell->material));
    int title_font_size = (int)(18 * dpi_scale);
    DrawText(info, panel_x + (int)(10 * dpi_scale), text_y, title_font_size, BLACK);
    text_y += (int)(20 * dpi_scale);
    
    // Basic properties
    sprintf(info, "Temp: %.1f°C | Density: %.2f | Pressure: %.2f", 
            cell->temperature, cell->density, cell->pressure);
    int props_font_size = (int)(16 * dpi_scale);
    DrawText(info, panel_x + (int)(10 * dpi_scale), text_y, props_font_size, BLACK);
    text_y += (int)(20 * dpi_scale);
    
    // Carried materials
    DrawText("Carried Materials:", panel_x + (int)(10 * dpi_scale), text_y, props_font_size, BLACK);
    text_y += (int)(20 * dpi_scale);
    
    bool has_carried = false;
    int detail_font_size = (int)(14 * dpi_scale);
    for (int i = 0; i < MATERIAL_COUNT; i++) {
        if (cell->material_quantities[i] > 0) {
            has_carried = true;
            sprintf(info, "- %s: %d%%", grid_material_name(i), 
                    (cell->material_quantities[i] * 100) / 255);
            DrawText(info, panel_x + (int)(20 * dpi_scale), text_y, detail_font_size, BLACK);
            text_y += (int)(16 * dpi_scale);
        }
    }
    
    if (!has_carried) {
        DrawText("None", panel_x + (int)(20 * dpi_scale), text_y, detail_font_size, BLACK);
    }
}

// ColorFromInt is now imported from color_utils.h
