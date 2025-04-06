#ifndef BUTTON_REGISTRY_H
#define BUTTON_REGISTRY_H

#include <stdbool.h>

// Register a button's location
void RegisterButtonLocation(int buttonID, int x, int y, int width, int height);

// Check if a mouse click is over a registered button
bool IsMouseOverButton(int buttonID, int mouseX, int mouseY);

// Clear all registered button locations
void ClearButtonRegistry(void);

#endif // BUTTON_REGISTRY_H