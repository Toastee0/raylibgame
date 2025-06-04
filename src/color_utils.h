/**
 * @file color_utils.h
 * @brief Utility functions for color manipulation
 */

#ifndef COLOR_UTILS_H
#define COLOR_UTILS_H

#include <raylib.h>
#include <stdint.h>

/**
 * @brief Convert an unsigned 32-bit RGBA color integer to a Raylib Color structure
 * 
 * @param color 32-bit color in RGBA format (0xRRGGBBAA)
 * @return Color Raylib Color structure
 */
Color ColorFromInt(uint32_t color);

#endif // COLOR_UTILS_H
