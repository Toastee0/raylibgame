#include "update_water.h"
#include "grid.h"
#include "cell_types.h"
#include "cell_actions.h"
#include <stdlib.h>

void UpdateWater(void) {
    bool processRightToLeft = GetRandomValue(0, 1);

    for (int y = GRID_HEIGHT - 2; y >= 1; y--) {
        if (processRightToLeft) {
            for (int x = GRID_WIDTH - 2; x >= 1; x--) { // Process right to left
                if (grid[y][x].type == CELL_TYPE_WATER) {
                    bool hasMoved = false;
                    grid[y][x].is_falling = false;

                    // Check if water can fall straight down
                    if (grid[y + 1][x].type == CELL_TYPE_AIR && y + 1 < GRID_HEIGHT - 1) {
                        MoveCell(x, y, x, y + 1);
                        hasMoved = true;
                    } 
                    // Check if water can fall diagonally
                    else {
                        bool canFallDiagonalLeft = (x > 1 && y + 1 < GRID_HEIGHT - 1 && grid[y + 1][x - 1].type == CELL_TYPE_AIR);
                        bool canFallDiagonalRight = (x < GRID_WIDTH - 2 && y + 1 < GRID_HEIGHT - 1 && grid[y + 1][x + 1].type == CELL_TYPE_AIR);

                        if (canFallDiagonalLeft && canFallDiagonalRight) {
                            int direction = (GetRandomValue(0, 100) < 50) ? -1 : 1;
                            MoveCell(x, y, x + direction, y + 1);
                            hasMoved = true;
                        } else if (canFallDiagonalLeft) {
                            MoveCell(x, y, x - 1, y + 1);
                            hasMoved = true;
                        } else if (canFallDiagonalRight) {
                            MoveCell(x, y, x + 1, y + 1);
                            hasMoved = true;
                        }
                    }

                    // Density sorting: water should sink below less dense materials
                    if (!hasMoved && y < GRID_HEIGHT - 1) {
                        if (grid[y + 1][x].type != CELL_TYPE_WATER && grid[y + 1][x].type != CELL_TYPE_AIR) {
                            MoveCell(x, y, x, y + 1);
                            hasMoved = true;
                        }
                    }

                    // Cohesion: water should try to stay together
                    if (!hasMoved) {
                        bool canMoveLeft = (x > 1 && grid[y][x - 1].type == CELL_TYPE_WATER);
                        bool canMoveRight = (x < GRID_WIDTH - 2 && grid[y][x + 1].type == CELL_TYPE_WATER);

                        if (canMoveLeft && canMoveRight) {
                            int direction = (GetRandomValue(0, 100) < 50) ? -1 : 1;
                            MoveCell(x, y, x + direction, y);
                        } else if (canMoveLeft) {
                            MoveCell(x, y, x - 1, y);
                        } else if (canMoveRight) {
                            MoveCell(x, y, x + 1, y);
                        }
                    }

                    // Prevent water from affecting border tiles
                    if (x == 1 || x == GRID_WIDTH - 2 || y == 1 || y == GRID_HEIGHT - 2) {
                        hasMoved = false;
                    }

                    // Update falling state based on movement
                    grid[y][x].is_falling = hasMoved;
                }
            }
        } else {
            for (int x = 1; x < GRID_WIDTH - 1; x++) { // Process left to right
                if (grid[y][x].type == CELL_TYPE_WATER) {
                    bool hasMoved = false;
                    grid[y][x].is_falling = false;

                    // Check if water can fall straight down
                    if (grid[y + 1][x].type == CELL_TYPE_AIR && y + 1 < GRID_HEIGHT - 1) {
                        MoveCell(x, y, x, y + 1);
                        hasMoved = true;
                    } 
                    // Check if water can fall diagonally
                    else {
                        bool canFallDiagonalLeft = (x > 1 && y + 1 < GRID_HEIGHT - 1 && grid[y + 1][x - 1].type == CELL_TYPE_AIR);
                        bool canFallDiagonalRight = (x < GRID_WIDTH - 2 && y + 1 < GRID_HEIGHT - 1 && grid[y + 1][x + 1].type == CELL_TYPE_AIR);

                        if (canFallDiagonalLeft && canFallDiagonalRight) {
                            int direction = (GetRandomValue(0, 100) < 50) ? -1 : 1;
                            MoveCell(x, y, x + direction, y + 1);
                            hasMoved = true;
                        } else if (canFallDiagonalLeft) {
                            MoveCell(x, y, x - 1, y + 1);
                            hasMoved = true;
                        } else if (canFallDiagonalRight) {
                            MoveCell(x, y, x + 1, y + 1);
                            hasMoved = true;
                        }
                    }

                    // Density sorting: water should sink below less dense materials
                    if (!hasMoved && y < GRID_HEIGHT - 1) {
                        if (grid[y + 1][x].type != CELL_TYPE_WATER && grid[y + 1][x].type != CELL_TYPE_AIR) {
                            MoveCell(x, y, x, y + 1);
                            hasMoved = true;
                        }
                    }

                    // Cohesion: water should try to stay together
                    if (!hasMoved) {
                        bool canMoveLeft = (x > 1 && grid[y][x - 1].type == CELL_TYPE_WATER);
                        bool canMoveRight = (x < GRID_WIDTH - 2 && grid[y][x + 1].type == CELL_TYPE_WATER);

                        if (canMoveLeft && canMoveRight) {
                            int direction = (GetRandomValue(0, 100) < 50) ? -1 : 1;
                            MoveCell(x, y, x + direction, y);
                        } else if (canMoveLeft) {
                            MoveCell(x, y, x - 1, y);
                        } else if (canMoveRight) {
                            MoveCell(x, y, x + 1, y);
                        }
                    }

                    // Prevent water from affecting border tiles
                    if (x == 1 || x == GRID_WIDTH - 2 || y == 1 || y == GRID_HEIGHT - 2) {
                        hasMoved = false;
                    }

                    // Update falling state based on movement
                    grid[y][x].is_falling = hasMoved;
                }
            }
        }
    }
}