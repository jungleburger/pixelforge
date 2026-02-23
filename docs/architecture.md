# PixelForge Architecture

## Overview

PixelForge is a 2D pixel-simulation engine built on a **hybrid particle-physics + crystalline lattice-bonding** model.

```
┌───────────────────────────────────────────────────────────────┐
│                        Application Layer                       │
│              editor/               sandbox/                    │
└───────────────────────────────────────────────────────────────┘
                           │
┌───────────────────────────────────────────────────────────────┐
│                     pixelforge_engine                          │
│                                                                │
│  ┌────────────┐  ┌─────────────┐  ┌──────────────────────┐   │
│  │   World    │  │ ParticleSystem│  │   WorldGenerator     │   │
│  │ ┌────────┐ │  │  (physics/) │  │    (procgen/)        │   │
│  │ │Lattice │ │  └─────────────┘  └──────────────────────┘   │
│  │ └────────┘ │                                               │
│  │ ┌────────┐ │  ┌─────────────┐  ┌──────────────────────┐   │
│  │ │BondMgr │ │  │ReactionSystem│  │      LuaApi          │   │
│  │ └────────┘ │  │ (reaction/) │  │   (scripting/)       │   │
│  └────────────┘  └─────────────┘  └──────────────────────┘   │
│                  ┌─────────────┐                              │
│                  │  GlRenderer │                              │
│                  │ (renderer/) │                              │
│                  └─────────────┘                              │
│                                                                │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │          ElementRegistry  (element/)                    │  │
│  └─────────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────┘
```

## Two Pixel Modes

### Dynamic Pixels (`pixel/dynamic_pixel.hpp`)
- Physically simulated at float precision
- Subject to: gravity (980 px/s²), linear drag, position integration
- Live in `Chunk::dynamic_pixels`

### Settled Pixels (`pixel/settled_pixel.hpp`)
- Bonded to integer grid coordinates
- Live in both `Chunk::cells` and the global `Lattice`
- Connected by `BondManager` bonds (N/S/E/W only)

## Settling Algorithm (`physics/particle_system.cpp`)

1. Round float pos → `(cx, cy)`
2. If `(cx, cy)` occupied → nudge velocity, stay dynamic
3. Check N/S/E/W neighbours in lattice
4. If compatible neighbour found OR `py >= world_floor` → settle, create bonds, mark chunk dirty
5. Else → apply random impulse ±5 px/s, stay dynamic

## Module Map

| Directory | Responsibility |
|-----------|----------------|
| `core/`   | Types, Logger, Assert |
| `element/`| ElementDef, ElementRegistry |
| `pixel/`  | DynamicPixel, SettledPixel |
| `bond/`   | Bond, BondManager |
| `lattice/`| Sparse integer-grid lookup |
| `physics/`| ParticleSystem (fixed timestep 1/60 s) |
| `reaction/`| ReactionSystem — temperature propagation, melting, boiling, ignition, solidification, contact reactions |
| `world/`  | World, Chunk |
| `procgen/`| WorldGenerator, BiomeRegistry, noise utilities |
| `renderer/`| IRenderer, GlRenderer, SettledLayer, DynamicLayer |
| `scripting/`| LuaApi (Sol2 Lua 5.4) |
| `save/`   | WorldSerialiser (zstd binary, format PFW2) |

## Data Flow

```
Lua scripts
    └─► ElementRegistry ─► World
                              │
                    ┌─────────┴──────────┐
                    │                    │
              Lattice (settled)   dynamic_pixels
                    │                    │
               BondManager         ParticleSystem
                    │         ┌──────────┘
               ReactionSystem─┘  (reads & mutates Lattice;
                    │             spawns new DynamicPixels)
                    │
              SettledLayer          DynamicLayer
                    └─────────┬──────────┘
                           GlRenderer
                                │
                    ┌───────────┴────────────┐
            WorldSerialiser            EditorApp (Phase 5)
              (zstd PFW2)                    │
                                   ┌─────────┴──────────┐
                              EditorContext         9 ImGui Panels
                          (shared state ptr)        3 Tools
```

---

## Editor Subsystem (`editor/`)

> Phase 5 — fully implemented.

The editor is a standalone `pixelforge_editor` executable that links `pixelforge_engine` and
layers Dear ImGui (docking mode) on top of the simulation loop.

### EditorContext

`EditorContext` is a lightweight struct (no ownership) passed by reference to every panel and
tool each frame. It holds raw pointers to the live engine objects and shared UI state:

| Field | Type | Purpose |
|-------|------|---------|
| `world` | `World*` | Lattice / chunk access |
| `registry` | `ElementRegistry*` | Element lookup |
| `reactions` | `ReactionSystem*` | Ambient temp, heat injection |
| `biomes` | `BiomeRegistry*` | Biome list for WorldgenPanel |
| `particles` | `ParticleSystem*` | Stat display in PerformancePanel |
| `selected_element` | `int` | Which element the palette has active |
| `brush_size` | `int` | Paint / erase radius |
| `active_tool` | `ActiveTool` | Paint / Erase / Select |
| `selection_box` | `optional<AABB>` | SelectTool result; read by HierarchyPanel |
| `heat_brush_active` | `bool` | Enable heat injection on click |
| `heat_brush_amount` | `float` | ΔT per click (°C) |
| `show_temp_overlay` | `bool` | Temperature colour gradient request |
| `save_path` | `string` | Path used by File → Save/Load |

### Panels

| Panel class | Key responsibility |
|-------------|-------------------|
| `ViewportPanel` | Camera (zoom/pan), tool dispatch, selection overlay draw |
| `HierarchyPanel` | Per-chunk stats table, selection AABB query |
| `InspectorPanel` | Full ElementDef read-out including all Phase 3/4 fields |
| `ElementPalettePanel` | Element list; sets `ctx.selected_element` |
| `ElementEditorPanel` | Collapsing-header view of physics, thermal, contact reactions |
| `WorldgenPanel` | Noise param widgets, biome list, calls `WorldGenerator::generate()` |
| `AssetBrowserPanel` | `std::filesystem` scanner, file preview, .pfw path setter |
| `ConsolePanel` | Scrolling log deque (max 512 lines) |
| `PerformancePanel` | FPS sparkline (120-frame history), pixel counts, thermal readout |

### Tools

Tools are owned by `ViewportPanel` and dispatched based on `ctx.active_tool`.

| Tool class | Interaction |
|------------|-------------|
| `PaintTool` | Sets `SettledPixel` in a square brush radius |
| `EraseTool` | Calls `World::remove_settled()` in a square brush radius |
| `SelectTool` | Drag to define `AABB`; writes `ctx.selection_box` |

### main loop (`EditorApp::run`)

```
while running:
    process_events()           ← SDL3 events, ImGui forwarding
    update(dt)                 ← particles, reactions, renderer upload
    render()                   ← ImGui frame: menu bar + 9 panels
                                 + ImGui_ImplOpenGL3_RenderDrawData
                                 + SDL_GL_SwapWindow```
