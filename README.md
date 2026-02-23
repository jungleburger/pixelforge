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
cmake -B build -G "Visual Studio 17 2022" -A x64 `
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
│   │   ├── reaction/       ReactionSystem (Phase 3)
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
| 3 | **Element Reactions** — temperature-driven chain reactions | ✅ Complete |
| 4 | **Scripting** — full Sol2 Lua bindings, hot-reload | 🔜 Next — bindings done, hot-reload remaining |
| 5 | **Editor** — full ImGui panel suite | 🟡 Partial — shell + stubs; console & perf panels functional |
| 6 | **Save/Load** — zstd world serialisation | 🟡 Partial — `WorldSerialiser` API exists, untested end-to-end |
| 7 | **Lighting** — per-pixel light emission & propagation | ⬜ Planned |
| 8 | **Audio** — SDL3 audio, element sound events | ⬜ Planned |
| 9 | **Networking** — deterministic lock-step multiplayer | ⬜ Planned |
| 10 | **Release** — packaging, Steam, itch.io | ⬜ Planned |

### Phase 4 — Scripting / Hot-reload (current focus)

Sol2 Lua bindings and element loading are functional. Remaining work:

- File-watcher to detect changes in `assets/elements/*.lua` at runtime
- In-place `ElementRegistry` patching without restarting the simulation
- Lua sandbox safety (remove `io`/`os` from runtime, keep for load phase only)
- `LuaApi` exposure of `ReactionSystem::apply_heat()` for scripted events

---

## Licence

MIT © 2026 PixelForge Contributors