/**
 * @file input.c
 * @brief Implementation of input handling functions
 */

#include "input.h"
#include "rendering.h"
#include "simulation.h"
#include <raylib.h>

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
        // Adjust zoom (faster zoom when holding shift)
        float zoom_factor = IsKeyDown(KEY_LEFT_SHIFT) ? 0.2f : 0.1f;
        input_state.zoom += wheel_movement * zoom_factor;
        
        // Limit zoom range
        if (input_state.zoom < 0.25f) input_state.zoom = 0.25f;
        if (input_state.zoom > 5.0f) input_state.zoom = 5.0f;
        
        // Apply zoom
        rendering_set_zoom(input_state.zoom);
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
    }
    
    // Handle material placement with left and right mouse buttons
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        int grid_x, grid_y;
        input_get_mouse_grid_pos(&grid_x, &grid_y);
        
        // Check if we're not clicking the UI
        if (GetMouseY() > 40) {
            // Add selected material at cursor position
            simulation_add_material(grid, grid_x, grid_y, *brush_size, *selected_material);
        } else {
            // Check for UI element clicks
            int button_size = 30;
            int padding = 5;
            int x_pos = padding;
            
            // Check material selection buttons
            for (int i = 0; i < MATERIAL_COUNT; i++) {
                Rectangle button_rect = { (float)x_pos, (float)padding, (float)button_size, (float)button_size };                if (CheckCollisionPointRec((Vector2){ (float)GetMouseX(), (float)GetMouseY() }, button_rect)) {
                    *selected_material = (CellMaterial)i;
                    break;
                }
                x_pos += button_size + padding;
            }
            
            // Check pause button
            const char* pause_text = *paused ? "PLAY" : "PAUSE";
            int text_width = MeasureText(pause_text, 20);
            Rectangle pause_rect = { (float)x_pos, (float)padding, (float)(text_width + padding * 2), (float)button_size };
            if (CheckCollisionPointRec((Vector2){ (float)GetMouseX(), (float)GetMouseY() }, pause_rect)) {
                *paused = !(*paused);
            }
            x_pos += text_width + padding * 3;
            
            // Check debug button
            const char* debug_text = *show_debug ? "DEBUG: ON" : "DEBUG: OFF";
            text_width = MeasureText(debug_text, 20);
            Rectangle debug_rect = { (float)x_pos, (float)padding, (float)(text_width + padding * 2), (float)button_size };
            if (CheckCollisionPointRec((Vector2){ (float)GetMouseX(), (float)GetMouseY() }, debug_rect)) {
                *show_debug = !(*show_debug);
            }
        }
    }
    
    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
        int grid_x, grid_y;
        input_get_mouse_grid_pos(&grid_x, &grid_y);
        
        // Check if we're not clicking the UI
        if (GetMouseY() > 40) {
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
