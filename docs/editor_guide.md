# PixelForge Editor Guide

## Overview

The PixelForge Editor is a Dear ImGui–based application for designing and testing pixel worlds.

## Launching

```bash
# After building (see README.md for full build instructions):
./build/editor/pixelforge_editor
```

## Layout

The editor uses ImGui docking. Panels can be freely dragged, tabbed, and resized.

| Panel | Description |
|-------|-------------|
| **Viewport** | World view; scroll to zoom, right-drag to pan |
| **Hierarchy** | Shows active chunks and lattice cell count |
| **Inspector** | Properties of the selected element |
| **Element Palette** | Click to select active element for painting |
| **Element Editor** | Edit element properties in-session |
| **World Gen** | Configure and trigger procedural generation |
| **Asset Browser** | Browse `assets/` directory |
| **Console** | Engine log output |
| **Performance** | FPS and frame time |

## Tools

Select a tool from the toolbar (keyboard shortcuts shown):

| Tool | Key | Action |
|------|-----|--------|
| Paint | `P` | Left-click / drag to place pixels |
| Erase | `E` | Left-click / drag to remove pixels |
| Select | `S` | Drag to select a rectangular region |

### Brush Size

Use the **Inspector** panel or `[` / `]` keys to decrease / increase brush size.

## Defining a New Element

1. Create `assets/elements/my_element.lua` (see [element_api.md](element_api.md))
2. Restart the editor — elements are loaded at startup
3. The element will appear in the **Element Palette**

## Saving and Loading Worlds

- **File → Save** — saves current world to `saves/world.pfw` (zstd-compressed binary)
- **File → Load** — opens a file picker to load a `.pfw` file

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+S` | Save world |
| `Ctrl+O` | Open world |
| `Space`  | Play / Pause simulation |
| `F1`     | Toggle grid overlay |
| `Ctrl+Z` | Undo (planned) |

## Tips

- Use **World Gen → Generate** to quickly fill the world with procedural terrain.
- The **Performance** panel's FPS counter helps identify slow element scripts.
- Hold `Shift` while painting to spawn **dynamic** (airborne) pixels instead of settling them immediately.
