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
| `reaction/`| ReactionSystem — temperature propagation, melting, boiling, ignition |
| `world/`  | World, Chunk |
| `procgen/`| WorldGenerator, BiomeRegistry, noise utilities |
| `renderer/`| IRenderer, GlRenderer, SettledLayer, DynamicLayer |
| `scripting/`| LuaApi (Sol2) |
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
```
