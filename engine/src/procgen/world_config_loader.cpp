#include <pixelforge/procgen/world_config_loader.hpp>
#include <pixelforge/core/logger.hpp>
#include <toml++/toml.hpp>

namespace pf {

std::optional<WorldConfig> load_world_config(const char* path) {
    toml::table tbl;
    try {
        tbl = toml::parse_file(path);
    } catch (const toml::parse_error& e) {
        PF_LOG_ERROR("WorldConfig: failed to parse '{}': {}", path, e.what());
        return std::nullopt;
    }

    WorldConfig cfg{};

    // ── [world] section ───────────────────────────────────────────────────
    auto world = tbl["world"];

    if (auto v = world["seed"].value<int64_t>())
        cfg.seed = static_cast<uint32_t>(*v);

    if (auto v = world["width"].value<int64_t>())
        cfg.width = static_cast<int>(*v);

    if (auto v = world["height"].value<int64_t>())
        cfg.height = static_cast<int>(*v);

    if (auto v = world["cave_density"].value<double>())
        cfg.cave_density = static_cast<float>(*v);

    if (auto v = world["water_level"].value<double>())
        cfg.water_level = static_cast<float>(*v);

    if (auto v = world["lava_depth"].value<double>())
        cfg.lava_depth = static_cast<float>(*v);

    PF_LOG_INFO("WorldConfig: loaded '{}' — {}×{} seed={} cave={:.2f} "
                "water={:.2f} lava={:.2f}",
                path, cfg.width, cfg.height, cfg.seed,
                cfg.cave_density, cfg.water_level, cfg.lava_depth);
    return cfg;
}

} // namespace pf
