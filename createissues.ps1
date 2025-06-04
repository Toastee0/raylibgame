# create_game_issues.ps1

# --- Configuration ---
$repoOwner = "Toastee0"
$repoName = "raylibgame"
# --- End Configuration ---

# Array of issues to create
# Each issue is a hashtable with 'title' and 'body'
$issues = @(
    @{
        title = "Implement Foundational Grid and Cell Structures"
        body  = "Define the core `Cell` structure and `Material` enum (as initially discussed and outlined in `Falling Sand Simulation Rules for C with Raylib.markdown`). Create `src/grid.h` and `src/grid.c` to manage grid data, dimensions (`GRID_WIDTH`, `GRID_HEIGHT`), initialization, and basic cell access. This is a prerequisite for most other simulation features. Files to consider: `src/grid.h`, `src/grid.c`, potentially updating `main.c` and `debug_utils.h`/`.c` to use these."
    },
    @{
        title = "Implement Basic Falling Sand Mechanics for Solid Materials"
        body  = "Develop the logic for solid materials (e.g., Sand, Soil) to fall downwards if the cell below is empty (Air) or a liquid they can displace. This should be based on the rules document. Files to consider: `src/simulation.c` (or a new `src/solid_physics.c`), `src/grid.h`/`.c`."
    },
    @{
        title = "Develop Water Simulation: Flow, Pressure, and Buoyancy"
        body  = "Implement water behavior, including flowing downwards and sideways to fill spaces. Introduce concepts of water pressure (e.g., based on depth) and how it might affect other elements or its own movement. Consider buoyancy for lighter materials in water. Files to consider: `src/simulation.c` (or a new `src/liquid_physics.c`), `src/grid.h`/`.c`."
    },
    @{
        title = "Implement Temperature Simulation and Phase Changes"
        body  = "Introduce a temperature property to cells. Implement logic for phase changes: water freezing into ice below a certain temperature and boiling into steam above another. Consider how temperature spreads between cells. Files to consider: `src/simulation.c`, `src/grid.h`/`.c` (add temperature to `Cell`), `src/cell_types.h` (add ICE, STEAM materials)."
    },
    @{
        title = "Develop Air Pressure and Basic Gas Flow"
        body  = "Simulate air pressure, potentially based on density or surrounding materials. Implement basic gas flow, including how gases like air or steam move and interact. Consider compression and pressure wakes as mentioned in the vision. Files to consider: `src/simulation.c` (or a new `src/gas_physics.c`), `src/grid.h`/`.c`."
    },
    @{
        title = "Create Evaporation, Condensation, and Precipitation Cycle"
        body  = "Implement the cycle where water evaporates into water vapor/steam (especially from surface water based on temperature), forms clouds (if applicable as a visual or simulated entity), and condenses back into liquid water (rain) under certain conditions. Files to consider: `src/simulation.c`, `src/grid.h`/`.c`."
    },
    @{
        title = "Implement Mineral Transport and Deposition"
        body  = "Allow materials like air and water to carry fractions of other materials (e.g., minerals, dust). Implement rules for how these carried materials are picked up, transported, and deposited in new locations. Files to consider: `src/simulation.c`, `src/grid.h`/`.c` (ensure `carried_fractions` in `Cell` is utilized)."
    },
    @{
        title = "Develop User Interaction Tools for Material Placement"
        body  = "Create UI elements and input handling (mouse clicks, keyboard shortcuts) to allow the user to select different material types and 'paint' them onto the grid, or remove existing materials. Files to consider: `src/input.c`, `src/rendering.c` (for UI elements), `main.c`."
    },
    @{
        title = "Enhance Debug Grid Output and Visualization"
        body  = "Further refine the JSON debug output (`debug_utils.c`) to be comprehensive. Consider adding an in-game debug view that can overlay cell information (type, density, temperature, pressure, carried fractions) directly onto the rendered grid. Files to consider: `src/debug_utils.c`, `src/debug_utils.h`, `src/rendering.c`."
    },
    @{
        title = "Advanced Gas/Liquid Dynamics (Oxygen Not Included style)"
        body  = "(More advanced/long-term) Explore and implement more complex gas and liquid dynamics, focusing on realistic pressure systems, gas mixing, and flow behaviors similar to those seen in games like 'Oxygen Not Included.' Files to consider: `src/simulation.c`, `src/liquid_physics.c`, `src/gas_physics.c`."
    },
    @{
        title = "Refine Build Process and Cross-Platform Support"
        body  = "Continuously review and refine the `Makefile` and build instructions in `README.md`. Test and ensure compatibility across target platforms (Windows, Linux, Web, Android). Files to consider: `Makefile`, `Makefile.Android`, `README.md`."
    }
)

# Loop through the issues and create them
foreach ($issue in $issues) {
    Write-Host "Creating issue: $($issue.title)"
    try {
        gh issue create --title "$($issue.title)" --body "$($issue.body)" --repo "$repoOwner/$repoName"
        Write-Host "Successfully created issue: $($issue.title)" -ForegroundColor Green
    } catch {
        Write-Host "Error creating issue: $($issue.title)" -ForegroundColor Red
        Write-Host $_.Exception.Message -ForegroundColor Red
    }
    Write-Host "---"
}

Write-Host "All issues processed."