#include "simulation.h"
#include "physics.h"
#include "grid.h"
#include <string.h>

//----------------------------------------------------------------------------------
// Main simulation update
//----------------------------------------------------------------------------------
void UpdateSimulation(void) {
    // Copy current grid to next grid
    memcpy(GetNextGrid(), GetGrid(), sizeof(Cell) * GRID_WIDTH * GRID_HEIGHT);
    
    // Update sand physics
    UpdateSand();
    
    // Update air pressure
    UpdatePressure();
    
    // Update temperature diffusion
    UpdateTemperature();
    
    // Update air movement based on temperature (buoyancy)
    UpdateAirMovement();
    
    // Copy next grid back to current grid
    SwapGrids();
}
