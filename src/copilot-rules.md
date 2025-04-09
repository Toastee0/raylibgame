# Copilot Rules for This Simulation

This document contains important rules and architectural guidelines for the cellular simulation. Copilot and humans alike should follow these.

---

## 🧠 Simulation Philosophy

- **Cells do not move.** Instead, properties (type, moisture, etc.) move from one cell to another. The position of a cell is static; its contents give the illusion of movement.
- **Integer simulation:** All simulation values, particularly moisture, are stored and transacted as integers to avoid floating-point errors. Floats may be used for display only.

---

## 🧱 Grid Structure

- The grid is a 2D array of `Cell` structures, defined in `grid.h`.
- Each cell is defined in `cell_types.h` and contains:
  - `type` — defines behavior and interactions.
  - `moisture` — integer between 0 and `moistureCapacity`.
  - `moistureCapacity` — the maximum moisture the cell can hold.
  - `temperature` — used for evaporation, freezing, and thermal effects.
  - `age` — used for lifecycle control (e.g., reproduction, decay).
  - `color` — for rendering only; has no effect on simulation.

---

## 💧 Moisture Rules

- **Moisture is stored per cell**, and different cell types have different capacities:
  - A **cell of air** can hold **up to 100 units** of moisture.
  - A **cell of water** can hold **up to 1000 units** of moisture.
- **Conversion rule:** It takes **10 fully saturated air cells** (10 × 100) to condense into **1 water cell** (1000 units).
- **Moisture is conserved**, and all transfers are done using **integers only** to ensure simulation stability.
- A **water cell with 0 moisture** becomes an **air cell**.
- **Partial water transfers** are allowed, but always use integer amounts.
- **Water cells can merge**:
  - If two neighboring water cells have a combined moisture amount that fits within one cell’s capacity (1000 units), they may merge into a single water cell.
- **Moisture also acts as a proxy for density** and can affect how fluids behave (e.g., settling or pushing other cells).

---

## 🌱 Cell Clumping & Group Behavior

- Certain cell types (e.g., soil) can clump together to simulate bulk interactions like pressure and cohesion.
- Clumps are discovered by walking the grid to find connected regions of the same type.
- Clumping allows:
  - Bulk movement (e.g., sand collapsing).
  - Region-based processing (e.g., pressure zones).
  - Smarter diffusion, cohesion, and behavior modeling.
- Example:
  - Soil clumps when moderately moist.
  - When very dry or saturated, it flows more freely or slumps under water pressure.

---

## 🧪 Example Interactions

- **Soil and Water:**
  - Thin sand columns can become saturated and slump under water pressure.
  - Larger sand masses resist water movement, but allow moisture diffusion.
  - Moisture tends to travel horizontally through soil like diffusion, forming water droplets at the surface over time.
- **Diffusion is conservative:**
  - Only transfer moisture that exists.
  - Always ensure the destination cell has capacity.
  - All moisture movement is tracked in integers.

---

## 🔁 Update Logic (cells.c)

- Each cell type may have a **support bitmask** to indicate where it expects structural support (e.g., below, sides).
- **Update strategies**:
  - Random left/right selection per row.
  - Checkerboard update pattern.
- **Cell updates follow this flow:**
  1. Check if the cell needs support.
  2. Use the support bitmask to determine stability.
  3. If movement is possible, select the preferred direction based on the cell’s rule set.
  4. Check that movement is valid and conserve properties (like moisture) during transfer.

---

## 🚫 Implementation Safeguards

- Never over-transfer moisture — ensure target cells have room.
- Conserve all quantities — no phantom moisture or loss.
- Use double-buffering or batch updates to avoid inconsistencies during simulation steps.

---

**Copilot: Do not ignore these rules. Use them to guide suggestions and avoid removing this document.**
