# PixelForge Editor Guide

## Overview

The PixelForge Editor is a Dear ImGui–based application for designing and testing pixel worlds.
It is built with ImGui docking — all panels can be freely dragged, tabbed, and resized.

## Launching

```bash
# After building (see README.md for full build instructions):
./build/editor/pixelforge_editor
```

---

## Panels

### Viewport

The main world canvas. The simulation renders behind the ImGui layer; the Viewport panel handles
all interaction input.

| Gesture | Action |
|---------|--------|
| Left-click / drag | Active tool (Paint / Erase / Select) |
| Scroll wheel | Zoom — pivots around the cursor position |
| Middle-mouse drag | Pan |
| Right-click | Clear current selection |

**Toolbar** (top of panel):

| Control | Description |
|---------|-------------|
| Tool buttons | Highlighted button = active tool; click to switch |
| Brush slider | Set brush radius (1–16 px) |
| Grid checkbox | Toggle pixel-grid overlay |
| Temp checkbox | Toggle temperature colour overlay (cold = blue → hot = red) |
| Heat checkbox | Enable heat-brush; slider sets ΔT per click (−500 … +2000 °C) |

### Hierarchy

Shows a live snapshot of the loaded world:

- **World summary** — dimensions, seed, chunk grid dimensions, total settled pixel count
- **Chunk table** — one row per chunk slot; columns: settled cell count, dynamic pixel count,
  sleeping flag (green = sleeping), dirty flag (orange = needs GPU upload)
- **Selection info** — when a Select-tool region is active, shows its origin, size,
  and how many settled pixels fall inside it (via lattice query)

### Inspector

Displays all properties of the currently selected `ElementDef`:

- Identity: name, tag, density
- Thermal: conductivity, heat output, melting / boiling / solidification / ignition points with
  product-element tags
- Contact reactions table: target, self→, other→, probability/s
- Heat-brush quick controls (mirrors the Viewport toolbar)

### Element Palette

Scrollable list of all registered elements. Click to change `selected_element`. The active item is
highlighted; all other panels (Viewport paint, Inspector, Element Editor) react immediately.

### Element Editor

Read-only tabbed view of the full `ElementDef` record for the selected element:

- **Physics** — density, viscosity, flammability
- **Temperature / Phase Changes** — thermal conductivity, heat output, melt/boil/solidify/ignite
  thresholds with unicode icons
- **Contact Reactions** — table of all contact-reaction rules

### World Gen

- Displays the current world's immutable config (seed, dimensions, cave density, water level, lava
  depth)
- **Noise parameter widgets** — terrain, cave, and ore noise layers (frequency, octaves,
  lacunarity, gain)
- **Registered Biomes** — lists all biomes probed from the `BiomeRegistry`
- **Regenerate World** button — clears the lattice then calls `WorldGenerator::generate()` using
  the current seed and tweaked noise params

### Asset Browser

- **Path bar** — editable root path; **Browse** scans it; **↺ Refresh** rescans
- **Quick-nav buttons** — jump to `elements/` or `worlds/` sub-directories
- **File list** (left column) — colour-coded by extension:
  - `.lua` — blue (element definitions)
  - `.toml` — yellow (world configs)
  - `.pfw` — green (world save files)
- **Preview pane** (right column) — first 24 lines of the selected file
- **Actions** by file type:
  - `.pfw` — sets the save path; use **File → Load World** to load
  - `.lua` / `.toml` — informational notes
- Status bar shows success / error messages

### Console

Scrolling log of engine messages (max 512 lines). Auto-scrolls to the bottom on new output.
Save/load operations and generator events write here.

### Performance

- **Frame** — current FPS, frame time in ms, 120-frame rolling average FPS
- **FPS sparkline** — `ImGui::PlotLines` graph of recent frame rate (0–200 fps range)
- **Simulation** — settled pixel count, dynamic pixel count, active chunk count
  (refreshed every 10 frames)
- **Thermal** — live read of `ReactionSystem::ambient_temperature` and `conduction_scale`

---

## Tools

| Tool | Key | Action |
|------|-----|--------|
| **Paint** | `P` | Left-click or drag to place settled pixels using the active element and brush size |
| **Erase** | `E` | Left-click or drag to remove settled pixels |
| **Select** | `B` | Drag to define a rectangular AABB; shown as a green overlay; right-click to clear |

---

## Defining a New Element

1. Create `assets/elements/my_element.lua` (see [element_api.md](element_api.md))
2. Restart the editor — elements are loaded at startup via `LuaApi`
3. The element will appear in the **Element Palette**

---

## Saving and Loading Worlds

Save state is managed through **File → Save World** / **File → Load World**.

- The current save path is shown in the File menu and editable inline.
- The **Asset Browser** can set the path by clicking a `.pfw` file → *Set as Save Path*,
  then use **File → Load World**.
- Saves use the zstd-compressed PFW2 binary format (`WorldSerialiser`).

| Shortcut | Action |
|----------|--------|
| `Ctrl+S` | Save world to current path |
| `Ctrl+L` | Load world from current path |

---

## Tips

- Use **World Gen → Regenerate World** to quickly fill the world with procedural terrain based on
  the current seed and tweaked noise parameters.
- The **Performance** panel's sparkline makes frame spikes easy to spot; high dynamic pixel counts
  are usually the culprit.
- The **Heat Brush** (enable in the Viewport toolbar) can trigger chain reactions on-demand —
  set a high positive value and click on a flammable pixel to ignite it.
- The **Selection overlay** (Select tool, key `B`) shows the selected region in both the Viewport
  and the Hierarchy panel, including a live count of settled pixels in the region.
