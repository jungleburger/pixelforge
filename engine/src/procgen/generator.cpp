#include <pixelforge/procgen/generator.hpp>
#include <pixelforge/procgen/noise_utils.hpp>
#include <pixelforge/core/logger.hpp>
#include <cmath>
#include <numbers>

namespace pf {

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

WorldGenerator::WorldGenerator(World& world, BiomeRegistry& biomes,
                                GeneratorConfig cfg)
    : m_world(world), m_biomes(biomes), m_cfg(std::move(cfg)),
      m_rng(world.config().seed)
{
    const uint32_t seed = world.config().seed;

    m_terrain_fnl.SetSeed(static_cast<int>(seed));
    m_terrain_fnl.SetNoiseType(m_cfg.terrain_noise.noise_type);
    m_terrain_fnl.SetFrequency(m_cfg.terrain_noise.frequency > 0.f
                               ? m_cfg.terrain_noise.frequency : 0.01f);

    m_cave_fnl.SetSeed(static_cast<int>(seed + 1));
    m_cave_fnl.SetNoiseType(m_cfg.cave_noise.noise_type);
    m_cave_fnl.SetFrequency(m_cfg.cave_noise.frequency > 0.f
                            ? m_cfg.cave_noise.frequency : 0.01f);

    m_ore_fnl.SetSeed(static_cast<int>(seed + 2));
    m_ore_fnl.SetNoiseType(m_cfg.ore_noise.noise_type);
    m_ore_fnl.SetFrequency(m_cfg.ore_noise.frequency > 0.f
                           ? m_cfg.ore_noise.frequency : 0.02f);

    m_biome_fnl.SetSeed(static_cast<int>(seed + 3));
    m_biome_fnl.SetNoiseType(m_cfg.biome_noise.noise_type);
    m_biome_fnl.SetFrequency(m_cfg.biome_noise.frequency);
}

// ─────────────────────────────────────────────────────────────────────────────
// Public entry point
// ─────────────────────────────────────────────────────────────────────────────

void WorldGenerator::generate() {
    PF_LOG_INFO("WorldGenerator: generating {}×{} world (seed {})…",
                m_world.config().width, m_world.config().height,
                m_world.config().seed);

    generate_terrain_columns();
    carve_worm_caves();
    place_underground_lakes();
    place_water_pockets();
    place_lava_pockets();
    place_gas_pockets();
    place_crystal_clusters();
    plant_initial_bonds();

    PF_LOG_INFO("WorldGenerator: done — lattice size = {}",
                m_world.lattice().size());
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

BiomeZone WorldGenerator::classify_zone(int wx) {
    const float n = m_biome_fnl.GetNoise(static_cast<float>(wx), 0.f);
    if (n < -0.33f) return BiomeZone::Cold;
    if (n >  0.33f) return BiomeZone::Arid;
    return BiomeZone::Temperate;
}

int WorldGenerator::surface_y_at(int wx) {
    const int   world_h = m_world.config().height;
    const float raw     = octave_noise(m_terrain_fnl,
                                       static_cast<float>(wx), 0.f,
                                       m_cfg.terrain_noise.octaves,
                                       m_cfg.terrain_noise.lacunarity,
                                       m_cfg.terrain_noise.gain);
    // Map [-1,1] → approximately 30–70 % of world height
    return static_cast<int>((raw * 0.5f + 0.5f) * world_h * 0.4f +
                            world_h * 0.3f);
}

void WorldGenerator::carve_circle(int cx, int cy, float radius) {
    const int w  = m_world.config().width;
    const int h  = m_world.config().height;
    const int r  = static_cast<int>(std::ceil(radius));
    const float r2 = radius * radius;

    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            if (static_cast<float>(dx*dx + dy*dy) > r2) continue;
            const int px = cx + dx;
            const int py = cy + dy;
            if (px < 0 || py < 0 || px >= w || py >= h) continue;
            m_world.remove_settled(px, py);
        }
    }
}

void WorldGenerator::fill_circle(int cx, int cy, float radius, ElementID elem) {
    const int w  = m_world.config().width;
    const int h  = m_world.config().height;
    const int r  = static_cast<int>(std::ceil(radius));
    const float r2 = radius * radius;
    auto&             reg = m_world.registry();
    const ElementDef* def = reg.get(elem);

    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            if (static_cast<float>(dx*dx + dy*dy) > r2) continue;
            const int px = cx + dx;
            const int py = cy + dy;
            if (px < 0 || py < 0 || px >= w || py >= h) continue;
            if (m_world.has_settled(px, py)) continue; // only fill air
            SettledPixel sp;
            sp.element   = elem;
            sp.world_pos = {px, py};
            if (def) sp.color = def->color;
            m_world.set_settled(px, py, sp);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Terrain generation — height map + depth bands + per-column biome zones
// ─────────────────────────────────────────────────────────────────────────────

void WorldGenerator::generate_terrain_columns() {
    for (int wx = 0; wx < m_world.config().width; ++wx)
        generate_terrain_column(wx);
}

void WorldGenerator::generate_terrain_column(int wx) {
    const int   world_h   = m_world.config().height;
    const int   surf_y    = surface_y_at(wx);
    const float subsoil_h = static_cast<float>(world_h - surf_y);
    const BiomeZone zone  = classify_zone(wx);
    auto& reg = m_world.registry();

    // Depth band boundaries (in absolute world y)
    const int dirt_end   = surf_y + static_cast<int>(subsoil_h * m_cfg.dirt_band_frac);
    const int deep_start = surf_y + static_cast<int>(
                           subsoil_h * (m_cfg.dirt_band_frac + m_cfg.stone_band_frac));

    for (int wy = surf_y; wy < world_h; ++wy) {
        const float depth_norm = (subsoil_h > 0.f)
            ? static_cast<float>(wy - surf_y) / subsoil_h
            : 1.f;

        const BiomeDef* biome = m_biomes.get_biome_at(depth_norm, zone);
        if (!biome) continue;

        // Macro cave threshold (Perlin worms add detail on top of this)
        const float cave = octave_noise(m_cave_fnl,
                                        static_cast<float>(wx),
                                        static_cast<float>(wy),
                                        m_cfg.cave_noise.octaves,
                                        m_cfg.cave_noise.lacunarity,
                                        m_cfg.cave_noise.gain);
        const float cave_thresh = (depth_norm > 0.5f)
            ? biome->cave_density + 0.1f
            : biome->cave_density;
        if (cave > cave_thresh * 2.f - 1.f) continue; // leave as air

        // ── Depth-band element selection ──────────────────────────────────
        ElementID elem;
        if (wy == surf_y) {
            elem = biome->surface_element;
        } else if (wy <= dirt_end && biome->dirt_element != 0) {
            elem = biome->dirt_element;
        } else if (wy >= deep_start) {
            elem = biome->deep_element;
        } else {
            elem = biome->fill_element;
        }

        // ── Ore veins (stone band only) ───────────────────────────────────
        if (biome->ore_element != 0 && wy > dirt_end && wy < deep_start) {
            const float ore = m_ore_fnl.GetNoise(static_cast<float>(wx),
                                                  static_cast<float>(wy));
            if (ore > 1.f - biome->ore_chance * 2.f)
                elem = biome->ore_element;
        }

        SettledPixel sp;
        sp.element   = elem;
        sp.world_pos = {wx, wy};
        if (const ElementDef* def = reg.get(elem)) sp.color = def->color;
        m_world.set_settled(wx, wy, sp);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Perlin-worm cave carving
// ─────────────────────────────────────────────────────────────────────────────

void WorldGenerator::carve_worm_caves() {
    const int world_w = m_world.config().width;
    const int world_h = m_world.config().height;

    std::uniform_int_distribution<int>   dist_x(0, world_w - 1);
    std::uniform_real_distribution<float> dist_depth(0.10f, 0.88f);
    std::uniform_real_distribution<float> dist_angle(
        0.f, 2.f * std::numbers::pi_v<float>);

    // Separate Perlin instance for worm steering (low-frequency smooth curves)
    FastNoiseLite worm_fnl;
    worm_fnl.SetSeed(static_cast<int>(m_world.config().seed + 99));
    worm_fnl.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    worm_fnl.SetFrequency(0.008f);

    for (int w = 0; w < m_cfg.worm_count; ++w) {
        const int   start_x   = dist_x(m_rng);
        const float depth_frac = dist_depth(m_rng);
        const int   surf_y    = surface_y_at(start_x);
        const int   start_y   = surf_y +
            static_cast<int>(depth_frac * static_cast<float>(world_h - surf_y));

        float px    = static_cast<float>(start_x);
        float py    = static_cast<float>(start_y);
        float angle = dist_angle(m_rng);

        for (int s = 0; s < m_cfg.worm_steps; ++s) {
            // Perlin-noise steering: gentle angular drift
            const float steer = worm_fnl.GetNoise(px + static_cast<float>(s) * 0.1f,
                                                   py + static_cast<float>(s) * 0.1f);
            angle += steer * 0.4f;

            // Flatten caves slightly (vertical step is half horizontal)
            px += std::cos(angle) * m_cfg.worm_step_px;
            py += std::sin(angle) * m_cfg.worm_step_px * 0.5f;

            // Bounds check
            if (px < 0.f || px >= static_cast<float>(world_w) ||
                py < 0.f || py >= static_cast<float>(world_h)) break;

            // Radius varies ± 1.5 px along worm for organic feel
            const float r = std::max(1.f,
                m_cfg.worm_radius +
                worm_fnl.GetNoise(px * 2.f, py * 2.f) * 1.5f);

            carve_circle(static_cast<int>(px), static_cast<int>(py), r);
        }
    }

    PF_LOG_DEBUG("WorldGenerator: carved {} worm caves", m_cfg.worm_count);
}

// ─────────────────────────────────────────────────────────────────────────────
// Underground lakes — wide shallow elliptical water bodies
// ─────────────────────────────────────────────────────────────────────────────

void WorldGenerator::place_underground_lakes() {
    const int world_w = m_world.config().width;
    const int world_h = m_world.config().height;
    // 60-px margin on each side requires at least 121 px wide
    if (world_w < 130 || world_h < 40) {
        PF_LOG_DEBUG("WorldGenerator: world too small for underground lakes — skipping");
        return;
    }

    const ElementID water_id = m_world.registry().id_of("water");
    if (water_id == 0) { PF_LOG_WARN("WorldGenerator: 'water' element not found — skipping lakes"); return; }
    const ElementDef* def = m_world.registry().get(water_id);

    std::uniform_int_distribution<int>   dist_x(60, world_w - 60);
    std::uniform_real_distribution<float> dist_depth(0.20f, 0.60f);
    std::uniform_int_distribution<int>   dist_rx(20, 50);
    std::uniform_int_distribution<int>   dist_ry( 6, 14);

    for (int i = 0; i < m_cfg.underground_lake_count; ++i) {
        const int cx     = dist_x(m_rng);
        const int surf_y = surface_y_at(cx);
        const int cy     = surf_y + static_cast<int>(
            dist_depth(m_rng) * static_cast<float>(world_h - surf_y));
        const int rx = dist_rx(m_rng);
        const int ry = dist_ry(m_rng);
        const float rx2 = static_cast<float>(rx * rx);
        const float ry2 = static_cast<float>(ry * ry);

        // First carve the ellipse
        for (int dy = -ry; dy <= ry; ++dy) {
            for (int dx = -rx; dx <= rx; ++dx) {
                if (static_cast<float>(dx*dx)/rx2 +
                    static_cast<float>(dy*dy)/ry2 > 1.f) continue;
                const int px = cx + dx;
                const int py = cy + dy;
                if (px < 0 || py < 0 || px >= world_w || py >= world_h) continue;
                m_world.remove_settled(px, py);
            }
        }
        // Fill the lower half with water
        for (int dy = 0; dy <= ry; ++dy) {
            for (int dx = -rx; dx <= rx; ++dx) {
                if (static_cast<float>(dx*dx)/rx2 +
                    static_cast<float>(dy*dy)/ry2 > 1.f) continue;
                const int px = cx + dx;
                const int py = cy + dy;
                if (px < 0 || py < 0 || px >= world_w || py >= world_h) continue;
                SettledPixel sp;
                sp.element   = water_id;
                sp.world_pos = {px, py};
                if (def) sp.color = def->color;
                m_world.set_settled(px, py, sp);
            }
        }
    }

    PF_LOG_DEBUG("WorldGenerator: placed {} underground lakes",
                 m_cfg.underground_lake_count);
}

// ─────────────────────────────────────────────────────────────────────────────
// Liquid / gas pockets
// ─────────────────────────────────────────────────────────────────────────────

void WorldGenerator::place_water_pockets() {
    const int world_w = m_world.config().width;
    const int world_h = m_world.config().height;
    if (world_w < 42) { PF_LOG_DEBUG("WorldGenerator: world too small for water pockets — skipping"); return; }

    const ElementID water_id = m_world.registry().id_of("water");
    if (water_id == 0) return;

    std::uniform_int_distribution<int>   dist_x(20, world_w - 20);
    std::uniform_real_distribution<float> dist_depth(0.08f, 0.52f);
    std::uniform_real_distribution<float> dist_r(3.f, 8.f);

    for (int i = 0; i < m_cfg.water_pocket_count; ++i) {
        const int   cx   = dist_x(m_rng);
        const int   surf = surface_y_at(cx);
        const int   cy   = surf + static_cast<int>(
            dist_depth(m_rng) * static_cast<float>(world_h - surf));
        const float r    = dist_r(m_rng);
        carve_circle(cx, cy, r);
        fill_circle (cx, cy, r - 1.f, water_id);
    }

    PF_LOG_DEBUG("WorldGenerator: placed {} water pockets", m_cfg.water_pocket_count);
}

void WorldGenerator::place_lava_pockets() {
    const int world_w = m_world.config().width;
    const int world_h = m_world.config().height;
    if (world_w < 42) { PF_LOG_DEBUG("WorldGenerator: world too small for lava pockets — skipping"); return; }

    const ElementID lava_id = m_world.registry().id_of("lava");
    if (lava_id == 0) return;

    std::uniform_int_distribution<int>   dist_x(20, world_w - 20);
    std::uniform_real_distribution<float> dist_depth(0.68f, 0.96f);
    std::uniform_real_distribution<float> dist_r(4.f, 10.f);

    for (int i = 0; i < m_cfg.lava_pocket_count; ++i) {
        const int   cx   = dist_x(m_rng);
        const int   surf = surface_y_at(cx);
        const int   cy   = surf + static_cast<int>(
            dist_depth(m_rng) * static_cast<float>(world_h - surf));
        const float r    = dist_r(m_rng);
        carve_circle(cx, cy, r);
        fill_circle (cx, cy, r - 1.f, lava_id);
    }

    PF_LOG_DEBUG("WorldGenerator: placed {} lava pockets", m_cfg.lava_pocket_count);
}

void WorldGenerator::place_gas_pockets() {
    const int world_w = m_world.config().width;
    const int world_h = m_world.config().height;
    if (world_w < 42) { PF_LOG_DEBUG("WorldGenerator: world too small for gas pockets — skipping"); return; }

    const ElementID smoke_id = m_world.registry().id_of("smoke");
    if (smoke_id == 0) return;

    std::uniform_int_distribution<int>   dist_x(20, world_w - 20);
    std::uniform_real_distribution<float> dist_depth(0.04f, 0.42f);
    std::uniform_real_distribution<float> dist_r(4.f, 9.f);

    for (int i = 0; i < m_cfg.gas_pocket_count; ++i) {
        const int   cx   = dist_x(m_rng);
        const int   surf = surface_y_at(cx);
        const int   cy   = surf + static_cast<int>(
            dist_depth(m_rng) * static_cast<float>(world_h - surf));
        const float r    = dist_r(m_rng);
        carve_circle(cx, cy, r);
        fill_circle (cx, cy, r - 1.f, smoke_id);
    }

    PF_LOG_DEBUG("WorldGenerator: placed {} gas pockets", m_cfg.gas_pocket_count);
}

// ─────────────────────────────────────────────────────────────────────────────
// Crystal clusters — spire formations in the deep zone
// ─────────────────────────────────────────────────────────────────────────────

void WorldGenerator::place_crystal_clusters() {
    const int world_w = m_world.config().width;
    const int world_h = m_world.config().height;

    const ElementID crystal_id = m_world.registry().id_of("crystal");
    if (crystal_id == 0) {
        PF_LOG_DEBUG("WorldGenerator: 'crystal' element not registered — skipping clusters");
        return;
    }
    auto&             reg = m_world.registry();
    const ElementDef* def = reg.get(crystal_id);

    std::uniform_int_distribution<int>   dist_x(10, world_w - 10);
    std::uniform_real_distribution<float> dist_depth(0.50f, 0.90f);
    // Spire height variation
    std::uniform_int_distribution<int>   dist_height(4, 9);
    // Number of spires per cluster (1–3)
    std::uniform_int_distribution<int>   dist_spires(1, 3);

    auto place_pixel = [&](int px, int py) {
        if (px < 0 || py < 0 || px >= world_w || py >= world_h) return;
        // Crystals grow on/through existing solids
        SettledPixel sp;
        sp.element   = crystal_id;
        sp.world_pos = {px, py};
        if (def) sp.color = def->color;
        m_world.set_settled(px, py, sp);
    };

    for (int i = 0; i < m_cfg.crystal_cluster_count; ++i) {
        const int cx     = dist_x(m_rng);
        const int surf_y = surface_y_at(cx);
        const int cy     = surf_y + static_cast<int>(
            dist_depth(m_rng) * static_cast<float>(world_h - surf_y));

        const int num_spires = dist_spires(m_rng);
        for (int sp = 0; sp < num_spires; ++sp) {
            // Offset each spire within the cluster
            std::uniform_int_distribution<int> off(-5, 5);
            const int ox = cx + off(m_rng);
            const int h  = dist_height(m_rng);

            // Main vertical spire (2 px wide)
            for (int s = 0; s < h; ++s) {
                place_pixel(ox,     cy - s);
                place_pixel(ox + 1, cy - s);
            }
            // Small flanking spires at ±3 px (shorter by 3)
            if (h > 3) {
                for (int s = 0; s < h - 3; ++s) {
                    place_pixel(ox - 3, cy - s);
                    place_pixel(ox + 4, cy - s);
                }
            }
        }
    }

    PF_LOG_DEBUG("WorldGenerator: placed {} crystal clusters",
                 m_cfg.crystal_cluster_count);
}

// ─────────────────────────────────────────────────────────────────────────────
// Bond initialisation — bond every settled neighbour pair after generation
// ─────────────────────────────────────────────────────────────────────────────

void WorldGenerator::plant_initial_bonds() {
    const int w = m_world.config().width;
    const int h = m_world.config().height;

    constexpr int dx[4]       = { 0,  0, 1, -1};
    constexpr int dy[4]       = {-1,  1, 0,  0};
    constexpr Direction dirs[4] = {Direction::North, Direction::South,
                                   Direction::East,  Direction::West};
    auto& bonds = m_world.bonds();

    for (int wy = 0; wy < h; ++wy) {
        for (int wx = 0; wx < w; ++wx) {
            auto* a = m_world.get_settled(wx, wy);
            if (!a || a->id == 0) continue;
            for (int i = 0; i < 4; ++i) {
                const int nx = wx + dx[i];
                const int ny = wy + dy[i];
                if (nx < 0 || ny < 0 || nx >= w || ny >= h) continue;
                auto* b = m_world.get_settled(nx, ny);
                if (!b || b->id == 0) continue;
                // Only create the bond in the positive direction to avoid duplicates
                if (i == 0 || i == 3) {
                    bonds.create_bond(a->id, b->id, dirs[i]);
                }
            }
        }
    }
}

} // namespace pf
