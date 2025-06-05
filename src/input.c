/**
 * @file input.c
 * @brief Implementation of input handling functions
 */

#include "input.h"
#include "rendering.h"
#include "simulation.h"
#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>

// Input state
static struct {
    Vector2 drag_start;
    Vector2 last_pan;
    bool dragging;
    float zoom;
    Vector2 pan;
} input_state = {
    .drag_start = {0},
    .last_pan = {0},
    .dragging = false,
    .zoom = 1.0f,
    .pan = {0}
};

bool input_init(void) {
    input_state.zoom = 1.0f;
    input_state.pan.x = 0.0f;
    input_state.pan.y = 0.0f;
    input_state.dragging = false;
    
    return true;
}

bool input_process(Grid* grid, CellMaterial* selected_material, int* brush_size, bool* paused, bool* show_debug) {
    if (!grid || !selected_material || !brush_size || !paused || !show_debug) return false;
    
    // Check for exit request
    if (WindowShouldClose() || IsKeyPressed(KEY_ESCAPE)) {
        return false;
    }
    
    // Handle pause/unpause with space
    if (IsKeyPressed(KEY_SPACE)) {
        *paused = !(*paused);
    }
    
    // Toggle debug mode with F1
    if (IsKeyPressed(KEY_F1)) {
        *show_debug = !(*show_debug);
    }
      // Material selection with number keys 0-9
    for (int i = 0; i <= 9; i++) {
        if (IsKeyPressed(KEY_ZERO + i)) {
            if (i < MATERIAL_COUNT) {
                *selected_material = (CellMaterial)i;
            }
        }
    }
    
    // Adjust brush size with [ and ]
    if (IsKeyPressed(KEY_LEFT_BRACKET)) {
        (*brush_size)--;
        if (*brush_size < 1) *brush_size = 1;
    }
    if (IsKeyPressed(KEY_RIGHT_BRACKET)) {
        (*brush_size)++;
        if (*brush_size > 20) *brush_size = 20;
    }
      // Handle zoom with mouse wheel
    float wheel_movement = GetMouseWheelMove();
    if (wheel_movement != 0) {
        // Get mouse position before zoom
        Vector2 mousePos = { (float)GetMouseX(), (float)GetMouseY() };
        
        // Convert mouse position to grid coordinates before zoom
        int grid_x_before, grid_y_before;
        rendering_screen_to_grid(mousePos.x, mousePos.y, &grid_x_before, &grid_y_before);
        
        // Store old zoom for calculations
        float old_zoom = input_state.zoom;
        
        // Adjust zoom (faster zoom when holding shift)
        float zoom_factor = IsKeyDown(KEY_LEFT_SHIFT) ? 0.2f : 0.1f;
        input_state.zoom += wheel_movement * zoom_factor;
        
        // Limit zoom range
        if (input_state.zoom < 0.25f) input_state.zoom = 0.25f;
        if (input_state.zoom > 5.0f) input_state.zoom = 5.0f;
        
        // Apply zoom
        rendering_set_zoom(input_state.zoom);
        
        // Calculate grid coordinates at mouse position after zoom
        int grid_x_after, grid_y_after;
        rendering_screen_to_grid(mousePos.x, mousePos.y, &grid_x_after, &grid_y_after);
        
        // Adjust pan to keep the point under the mouse fixed during zoom
        input_state.pan.x += (grid_x_after - grid_x_before) * rendering_get_cell_size() * input_state.zoom;
        input_state.pan.y += (grid_y_after - grid_y_before) * rendering_get_cell_size() * input_state.zoom;
        
        // Apply the adjusted pan
        rendering_set_pan(input_state.pan.x, input_state.pan.y);
    }
    
    // Handle panning with middle mouse button
    if (IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON)) {
        input_state.drag_start = (Vector2){ (float)GetMouseX(), (float)GetMouseY() };
        input_state.last_pan = input_state.pan;
        input_state.dragging = true;
    }
    
    if (input_state.dragging) {
        if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
            input_state.pan.x = input_state.last_pan.x + (GetMouseX() - input_state.drag_start.x);
            input_state.pan.y = input_state.last_pan.y + (GetMouseY() - input_state.drag_start.y);
            rendering_set_pan(input_state.pan.x, input_state.pan.y);
        } else {
            input_state.dragging = false;
        }
    }    // Handle material placement with left mouse button
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        int grid_x, grid_y;
        input_get_mouse_grid_pos(&grid_x, &grid_y);
        
        // Calculate the UI bar height based on DPI scale
        float dpi_scale = rendering_get_dpi_scale();
        int button_size = (int)(30 * dpi_scale);
        int padding = (int)(5 * dpi_scale);
        int top_bar_height = button_size + padding * 2;
        
        // Check if we're not clicking the UI
        if (GetMouseY() > top_bar_height) {
            // Add selected material at cursor position
            simulation_add_material(grid, grid_x, grid_y, *brush_size, *selected_material);
        }
    }
      // Handle UI button clicks (use IsMouseButtonPressed to prevent rapid toggling)
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        // Get DPI scale from rendering module
        float dpi_scale = rendering_get_dpi_scale();
        int button_size = (int)(30 * dpi_scale);
        int padding = (int)(5 * dpi_scale);
        int top_bar_height = button_size + padding * 2;
        
        // Check if we're clicking the UI
        if (GetMouseY() <= top_bar_height) {
            int x_pos = padding;
          // Check material selection buttons
        for (int i = 0; i < MATERIAL_COUNT; i++) {
            Rectangle button_rect = { (float)x_pos, (float)padding, (float)button_size, (float)button_size };
            if (CheckCollisionPointRec((Vector2){ (float)GetMouseX(), (float)GetMouseY() }, button_rect)) {
                *selected_material = (CellMaterial)i;
                break;
            }
            x_pos += button_size + padding;
        }
                
        // Skip past brush size indicator
        char brush_text[32];
        sprintf(brush_text, "Brush: %d", *brush_size);
        int brush_font_size = (int)(20 * dpi_scale);
        x_pos += MeasureText(brush_text, brush_font_size) + padding * 2;
          
        // Check pause button
        const char* pause_text = *paused ? "PLAY" : "PAUSE";
        int button_font_size = (int)(20 * dpi_scale);
        int text_width = MeasureText(pause_text, button_font_size);
        Rectangle pause_rect = { (float)x_pos, (float)padding, (float)(text_width + padding * 2), (float)button_size };
        if (CheckCollisionPointRec((Vector2){ (float)GetMouseX(), (float)GetMouseY() }, pause_rect)) {
            *paused = !(*paused);
        }
        
        // Store x position after pause button for debug button
        int debug_x_pos = x_pos + text_width + padding * 3;
          // Check debug button
        const char* debug_text = *show_debug ? "DEBUG: ON" : "DEBUG: OFF";
        text_width = MeasureText(debug_text, button_font_size);
        Rectangle debug_rect = { (float)debug_x_pos, (float)padding, (float)(text_width + padding * 2), (float)button_size };
        if (CheckCollisionPointRec((Vector2){ (float)GetMouseX(), (float)GetMouseY() }, debug_rect)) {            *show_debug = !(*show_debug);
            }
        }
    }
      if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
        int grid_x, grid_y;
        input_get_mouse_grid_pos(&grid_x, &grid_y);
        
        // Calculate the UI bar height based on DPI scale
        float dpi_scale = rendering_get_dpi_scale();
        int button_size = (int)(30 * dpi_scale);
        int padding = (int)(5 * dpi_scale);
        int top_bar_height = button_size + padding * 2;
        
        // Check if we're not clicking the UI
        if (GetMouseY() > top_bar_height) {
            // Remove material (replace with air) at cursor position
            simulation_add_material(grid, grid_x, grid_y, *brush_size, MATERIAL_AIR);
        }
    }
    
    // Continue running
    return true;
}

void input_get_mouse_grid_pos(int* grid_x, int* grid_y) {
    int screen_x = GetMouseX();
    int screen_y = GetMouseY();
    rendering_screen_to_grid(screen_x, screen_y, grid_x, grid_y);
}
