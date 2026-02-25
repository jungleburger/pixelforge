# PixelForge

> A hybrid particle-physics + crystalline lattice-bonding 2D pixel simulation engine.

PixelForge is **not** a falling-sand clone. Every pixel lives in one of two states:

| Mode | Description |
|------|-------------|
| **Dynamic** | Free-body physics — gravity (980 px/s²), drag, angular momentum, collision |
| **Settled** | Bonded to the integer crystalline lattice via N/S/E/W bonds |

When a dynamic pixel comes to rest next to a settled neighbour (or hits the world floor), it *settles* — forming bonds with all touching lattice pixels and becoming part of the static simulation layer.

---

## Tech Stack

| Technology | Version | Role |
|------------|---------|------|
| C++ | 23 | Language |
| CMake | 3.25+ | Build system |
| vcpkg | latest | Package management |
| SDL3 | 3.x | Window, input |
| OpenGL | 4.3 Core | Rendering |
| Dear ImGui (docking) | latest | Editor UI only |
| Sol2 + Lua | 5.4 | Element scripting |
| GLM | 1.x | Math |
| FastNoiseLite | 1.x | Procedural noise (vendored) |
| zstd | 1.5+ | World save compression |
| Catch2 | 3.x | Unit tests |
| nlohmann-json | 3.x | JSON asset loading |
| toml++ | 3.x | World config files |

---

## Building

### Prerequisites

- CMake ≥ 3.25
- A C++23-capable compiler (GCC 13+, Clang 17+, MSVC 19.38+)
- [vcpkg](https://github.com/microsoft/vcpkg) installed and `VCPKG_ROOT` set

### Windows

```powershell
git clone https://github.com/jungleburger/pixelforge
cd pixelforge
cmake -B build -G "Visual Studio 18 2026" -A x64 `
    -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release
```

### macOS

```bash
git clone https://github.com/jungleburger/pixelforge
cd pixelforge
cmake -B build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build -j$(sysctl -n hw.logicalcpu)
```

### Linux

```bash
git clone https://github.com/jungleburger/pixelforge
cd pixelforge
cmake -B build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build -j$(nproc)
```

---

## Running

### Editor

```bash
./build/editor/pixelforge_editor
```

### Sandbox (headless demo)

```bash
cd build/sandbox
./pixelforge_sandbox
```

---

## Defining a New Element in Lua

Create a file in `sandbox/assets/elements/` (or your custom asset path):

```lua
-- assets/elements/copper.lua
define_element {
    name          = "Copper",
    tag           = "copper",
    physics       = "solid",
    color         = 0xB87333FF,
    density       = 8.96,
    melting_point = 1358.0,
    melt_into     = "lava",
}
```

See [docs/element_api.md](docs/element_api.md) for all available fields.

---

## Repository Structure

```
pixelforge/
├── CMakeLists.txt          Root build
├── vcpkg.json              Dependency manifest
├── engine/                 pixelforge_engine static library
│   ├── include/pixelforge/ Public C++ headers
│   │   ├── core/           Types, Logger, Assert
│   │   ├── element/        ElementDef, ElementRegistry
│   │   ├── pixel/          DynamicPixel, SettledPixel
│   │   ├── bond/           Bond, BondManager
│   │   ├── lattice/        Lattice (sparse integer grid)
│   │   ├── physics/        ParticleSystem
│   │   ├── reaction/       ReactionSystem — temperature, solidification, contact reactions
│   │   ├── world/          World, Chunk
│   │   ├── procgen/        WorldGenerator, BiomeRegistry
│   │   ├── renderer/       IRenderer, GlRenderer, layers
│   │   ├── scripting/      LuaApi
│   │   └── save/           WorldSerialiser
│   ├── src/                Implementation files
│   └── vendor/             FastNoiseLite (vendored)
├── editor/                 Dear ImGui editor app
│   └── src/
│       ├── panels/         9 ImGui panels
│       └── tools/          3 interaction tools
├── sandbox/                Headless demo app
│   └── assets/
│       ├── elements/       13 Lua element definitions
│       └── worlds/         TOML world configs
├── tests/                  Catch2 unit tests
└── docs/                   Architecture & API docs
```

---

## 10-Phase Roadmap

| Phase | Title | Status |
|-------|-------|--------|
| 1 | **Foundation** — types, lattice, bond system, physics, procgen | ✅ Complete |
| 2 | **Renderer** — GL texture pipeline, dynamic point sprites | ✅ Complete |
| 3 | **Element Reactions** — temperature propagation, melting, boiling, ignition, fire/lava heat emission | ✅ Complete |
| 4 | **Phase Changes & Contact Reactions** — solidification/condensation (lava→stone, steam→water, water→ice), contact reaction table, full Sol2 Lua bindings (`solidify_point`, `reactive_with`), editor integration (heat brush, temp overlay, InspectorPanel thermal display) | ✅ Complete |
| 5 | **Editor** — full ImGui panel suite | ✅ Complete |
| 6 | **Procedural World Generator** — noise-based terrain, biomes, cave carving, feature placement | ✅ Complete |
| 7 | **Save/Load** — zstd world serialisation | ✅ Complete |
| 8 | **Scripting** — Lua hot-reload, modding API | ⬜ Planned |
| 9 | **Lighting** — per-pixel light emission & propagation | ⬜ Planned |
| 10 | **Release** — packaging, Steam, itch.io | ⬜ Planned |

### Phase 6 — Procedural World Generator ✅

Fully implemented. Key additions to `WorldGenerator`, `BiomeRegistry`, and the engine:

- **Simplex noise heightmap** with 4-octave FBm; surface height spans 30–70 % of world height
- **Horizontal biome zones** — `classify_zone(wx)` samples a low-frequency Perlin field to assign
  columns to *Temperate*, *Cold*, or *Arid* climate zones, each with distinct surface/fill elements
- **Terrain depth bands** — surface row, dirt/ice/sand subsurface band (~6 % of depth),
  stone bulk (~82 %), bedrock/lava deep zone (~12 %)
- **Perlin-worm cave carving** — configurable worm count/steps/radius; worms steer via a
  dedicated Perlin FNL for smooth, organic tunnel shapes
- **Material injection** — water pockets (shallow), lava pockets (deep), gas (smoke) pockets
- **Feature placement** — elliptical underground lakes (carved + water-filled), multi-spire
  crystal clusters using the new `crystal` element
- **Bond initialisation** — every placed settled pixel bonds all valid N/S/E/W neighbours
- **TOML world-config loader** — `load_world_config(path)` via toml++; sandbox auto-loads
  `assets/worlds/sandbox_world.toml` with a fallback to hardcoded defaults
- **New elements**: `dirt` (powder, brown subsurface), `crystal` (solid, light-emitting, deep ore)

### Phase 7 — Save/Load ✅

Fully implemented. Key additions:

- **Binary format v2** (`PFW2`) — flat `CellRecord` array (position, element ID, temperature, hp,
  colour) prefixed with a `Header` (magic, dimensions, seed, cell count)
- **zstd compression** — level 3; typical world saves compress 10–30× smaller than raw
- **`WorldSerialiser::save`** — iterates `lattice().query_rect(full_world_AABB)`, serialises all
  settled pixels, compresses with `ZSTD_compress`, and writes to disk
- **`WorldSerialiser::load`** — reads file, decompresses, validates magic, rebuilds `World` and
  re-populates the lattice via `world.set_settled`
- **Editor integration** — File menu Save (`Ctrl+S`) / Load (`Ctrl+L`) with inline path field;
  Asset Browser highlights `.pfw` files and sets save path with one click
- **Round-trip test suite** (`test_save.cpp`) — 8 Catch2 tests: empty world, pixel attribute
  preservation, config fields, large world (5 000 cells) spot-check, overwrite semantics,
  save/load error paths, and corrupt-data rejection

---

## Licence

MIT © 2026 PixelForge Contributors