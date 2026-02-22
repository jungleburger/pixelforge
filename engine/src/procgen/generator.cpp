#include <pixelforge/procgen/generator.hpp>
#include <pixelforge/procgen/noise_utils.hpp>
#include <pixelforge/core/logger.hpp>
#include <cmath>

namespace pf {

WorldGenerator::WorldGenerator(World& world, BiomeRegistry& biomes,
                                GeneratorConfig cfg)
    : m_world(world), m_biomes(biomes), m_cfg(cfg)
{
    const uint32_t seed = world.config().seed;

    m_terrain_fnl.SetSeed(static_cast<int>(seed));
    m_terrain_fnl.SetNoiseType(m_cfg.terrain_noise.noise_type);
    m_terrain_fnl.SetFrequency(m_cfg.terrain_noise.frequency);

    m_cave_fnl.SetSeed(static_cast<int>(seed + 1));
    m_cave_fnl.SetNoiseType(m_cfg.cave_noise.noise_type);
    m_cave_fnl.SetFrequency(m_cfg.cave_noise.frequency);

    m_ore_fnl.SetSeed(static_cast<int>(seed + 2));
    m_ore_fnl.SetNoiseType(m_cfg.ore_noise.noise_type);
    m_ore_fnl.SetFrequency(m_cfg.ore_noise.frequency);
}

void WorldGenerator::generate() {
    PF_LOG_INFO("WorldGenerator: generating {}x{} world…",
                m_world.config().width, m_world.config().height);

    for (int wx = 0; wx < m_world.config().width; ++wx) {
        generate_terrain_column(wx);
    }

    plant_initial_bonds();

    PF_LOG_INFO("WorldGenerator: done. Lattice size = {}",
                m_world.lattice().size());
}

void WorldGenerator::generate_terrain_column(int wx) {
    const int   world_h = m_world.config().height;
    const float nx      = static_cast<float>(wx);

    // Terrain height: 0.3–0.7 of world height
    const float raw     = octave_noise(m_terrain_fnl, nx, 0.f,
                                       m_cfg.terrain_noise.octaves,
                                       m_cfg.terrain_noise.lacunarity,
                                       m_cfg.terrain_noise.gain);
    const int surface_y = static_cast<int>(
        (raw * 0.5f + 0.5f) * world_h * 0.4f + world_h * 0.3f);

    auto& reg = m_world.registry();

    for (int wy = surface_y; wy < world_h; ++wy) {
        const float depth_norm = static_cast<float>(wy - surface_y) /
                                 static_cast<float>(world_h - surface_y);

        const BiomeDef* biome = m_biomes.get_biome_at(depth_norm);
        if (!biome) continue;

        // Cave carving
        const float cave = octave_noise(m_cave_fnl,
                                        static_cast<float>(wx),
                                        static_cast<float>(wy),
                                        m_cfg.cave_noise.octaves,
                                        m_cfg.cave_noise.lacunarity,
                                        m_cfg.cave_noise.gain);
        const float cave_thresh = (depth_norm > 0.5f) ?
            biome->cave_density + 0.1f : biome->cave_density;
        if (cave > cave_thresh * 2.f - 1.f) continue; // leave as air

        // Choose element
        ElementID elem;
        if (wy == surface_y) {
            elem = biome->surface_element;
        } else if (depth_norm > 0.9f) {
            elem = biome->deep_element;
        } else {
            elem = biome->fill_element;
        }

        // Ore veins
        if (biome->ore_element != 0) {
            const float ore = m_ore_fnl.GetNoise(static_cast<float>(wx),
                                                  static_cast<float>(wy));
            if (ore > 1.f - biome->ore_chance * 2.f) {
                elem = biome->ore_element;
            }
        }

        SettledPixel sp;
        sp.element   = elem;
        sp.world_pos = {wx, wy};
        const ElementDef* def = reg.get(elem);
        if (def) sp.color = def->color;

        m_world.set_settled(wx, wy, sp);
    }
}

void WorldGenerator::plant_initial_bonds() {
    // Bond all N/S/E/W neighbours that have been placed.
    const int w = m_world.config().width;
    const int h = m_world.config().height;

    const int dx[4] = { 0,  0, 1, -1};
    const int dy[4] = {-1,  1, 0,  0};
    const Direction dirs[4] = {Direction::North, Direction::South,
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
                // Only bond in the positive direction to avoid duplicates
                if (i == 0 || i == 3) {
                    bonds.create_bond(a->id, b->id, dirs[i]);
                }
            }
        }
    }
}

} // namespace pf
