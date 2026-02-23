# Element API Reference

Elements are defined in Lua files and loaded at startup via `LuaApi::load_element_file()`.

## `define_element { … }`

Call `define_element` with a table of fields to register a new element.

### Required fields

| Field | Type | Description |
|-------|------|-------------|
| `name` | string | Display name (e.g. `"Sand"`) |
| `tag`  | string | Unique lowercase key (e.g. `"sand"`) |

### Optional fields

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `physics` | string | `"solid"` | One of `solid`, `powder`, `liquid`, `gas`, `fire`, `plasma`, `energy` |
| `color` | integer | `0xFFAA88FF` | RGBA packed as `0xRRGGBBAA` |
| `density` | float | `1.0` | Mass per unit volume |
| `viscosity` | float | `0.0` | 0 = free-flowing, 1 = fully viscous |
| `flammability` | float | `0.0` | Probability of igniting each tick (0–1) |
| `melting_point` | float | `-1` | Temperature (K) at which element converts to `melt_into` |
| `boiling_point` | float | `-1` | Temperature (K) at which element converts to `boil_into` |
| `melt_into` | string | — | Tag of the element to become when melting |
| `boil_into` | string | — | Tag of the element to become when boiling/evaporating |
| `emits_light` | bool | `false` | Whether this element emits light |
| `light_radius` | float | `0.0` | Radius of emitted light in pixels |
| `light_color` | integer | `0xFFFFFFFF` | RGBA colour of emitted light |

### Return value

`define_element` returns the numeric `ElementID` assigned to the new element.

## Example

```lua
-- my_element.lua
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

## Reaction Chains

Reactions between touching pixels are triggered by the physics system. The following chains are built-in (implemented via temperature and phase transitions):

| Trigger | Result |
|---------|--------|
| water + lava | → steam + stone |
| sand + lava | → glass + lava |
| fire + wood | → fire + smoke |
| fire + gunpowder | → fire + fire (chain, p=0.95) |
| acid + stone | → acid + air |
| ice temperature < 273 K | → water |
| lava temperature < 800 K | → stone |
| steam temperature < 373 K | → water |

## Helper Lua functions

| Function | Description |
|----------|-------------|
| `pf_log(msg)` | Emit a message to the PixelForge logger at INFO level |
