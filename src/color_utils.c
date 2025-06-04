/**
 * @file color_utils.c
 * @brief Implementation of color utility functions
 */

#include "color_utils.h"

/**
 * @brief Convert an unsigned 32-bit RGBA color integer to a Raylib Color structure
 * 
 * @param color 32-bit color in RGBA format (0xRRGGBBAA)
 * @return Color Raylib Color structure
 */
Color ColorFromInt(uint32_t color) {
    Color result;
    result.r = (color >> 24) & 0xFF;  // Red component (highest byte)
    result.g = (color >> 16) & 0xFF;  // Green component
    result.b = (color >> 8) & 0xFF;   // Blue component
    result.a = color & 0xFF;          // Alpha component (lowest byte)
    return result;
}
