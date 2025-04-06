#include "button_registry.h"
#include <raylib.h>
#include <stdlib.h>
#include <string.h>

// Define a structure to store button information
typedef struct {
    int buttonID;
    int x;
    int y;
    int width;
    int height;
} ButtonInfo;

// Array to store registered buttons
#define MAX_BUTTONS 10
static ButtonInfo buttonRegistry[MAX_BUTTONS];
static int buttonCount = 0;

// Register a button's location
void RegisterButtonLocation(int buttonID, int x, int y, int width, int height) {
    if (buttonCount >= MAX_BUTTONS) {
        return; // No more space to register buttons
    }

    buttonRegistry[buttonCount].buttonID = buttonID;
    buttonRegistry[buttonCount].x = x;
    buttonRegistry[buttonCount].y = y;
    buttonRegistry[buttonCount].width = width;
    buttonRegistry[buttonCount].height = height;
    buttonCount++;
}

// Check if a mouse click is over a registered button
bool IsMouseOverButton(int buttonID, int mouseX, int mouseY) {
    for (int i = 0; i < buttonCount; i++) {
        if (buttonRegistry[i].buttonID == buttonID) {
            if (mouseX > buttonRegistry[i].x && mouseX < (buttonRegistry[i].x + buttonRegistry[i].width) &&
                mouseY > buttonRegistry[i].y && mouseY < (buttonRegistry[i].y + buttonRegistry[i].height)) {
                return true;
            }
        }
    }
    return false;
}

// Clear all registered button locations
void ClearButtonRegistry(void) {
    buttonCount = 0; // Reset the button count to clear the registry
}