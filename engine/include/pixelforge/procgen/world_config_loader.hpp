#pragma once
#include <pixelforge/world/world.hpp>
#include <optional>

namespace pf {

/// Load a WorldConfig from a TOML file (e.g. `assets/worlds/default_world.toml`).
///
/// Returns the parsed config on success, or std::nullopt if the file cannot be
/// opened or contains parse errors (errors are emitted via PF_LOG_ERROR).
///
/// Example TOML layout:
/// ```toml
/// [world]
/// name         = "My World"
/// seed         = 42
/// width        = 4096
/// height       = 2048
/// cave_density = 0.35
/// water_level  = 0.45
/// lava_depth   = 0.82
/// ```
[[nodiscard]] std::optional<WorldConfig> load_world_config(const char* path);

} // namespace pf
