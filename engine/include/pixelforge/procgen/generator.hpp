#pragma once
#include <pixelforge/world/world.hpp>
#include <pixelforge/procgen/biome.hpp>
#include <pixelforge/procgen/noise_utils.hpp>
#include "FastNoiseLite/FastNoiseLite.h"
#include <random>

namespace pf {

struct GeneratorConfig {
    NoiseParams terrain_noise;
    NoiseParams cave_noise;
    NoiseParams ore_noise;
    /// x-axis sampling for horizontal climate zone assignment.
    /// Low frequency keeps zones wide (hundreds of pixels per transition).
    NoiseParams biome_noise{ .frequency = 0.002f };

    // ── Cave worm parameters ──────────────────────────────────────────────
    int   worm_count   = 40;   ///< Number of cave worms to carve
    int   worm_steps   = 200;  ///< Steps (pixels * step_px) per worm
    float worm_step_px = 2.f;  ///< Pixels advanced per step
    float worm_radius  = 3.5f; ///< Base tunnel radius (varied by noise)

    // ── Pocket / feature counts ───────────────────────────────────────────
    int   water_pocket_count    = 20;
    int   lava_pocket_count     = 12;
    int   gas_pocket_count      = 10;
    int   crystal_cluster_count = 25;
    int   underground_lake_count = 4;

    // ── Terrain depth-band fractions (of total subsurface height) ─────────
    float dirt_band_frac  = 0.06f;  ///< Top ~6 % of subsurface → dirt/sand/ice
    float stone_band_frac = 0.82f;  ///< Next ~82 % → stone fill
    // Remaining ~12 % → deep element (lava / bedrock)
};

class WorldGenerator {
public:
    WorldGenerator(World& world, BiomeRegistry& biomes,
                   GeneratorConfig cfg = {});

    /// Fill the whole world with procedurally generated content.
    void generate();

private:
    World&           m_world;
    BiomeRegistry&   m_biomes;
    GeneratorConfig  m_cfg;
    FastNoiseLite    m_terrain_fnl;
    FastNoiseLite    m_cave_fnl;
    FastNoiseLite    m_ore_fnl;
    FastNoiseLite    m_biome_fnl;   ///< Horizontal biome zone noise
    std::mt19937     m_rng;         ///< Seeded RNG for stochastic feature placement

    // ── Terrain generation ────────────────────────────────────────────────
    void generate_terrain_columns();
    void generate_terrain_column(int wx);

    // ── Cave carving via Perlin worms ─────────────────────────────────────
    void carve_worm_caves();

    // ── Feature / material injection ─────────────────────────────────────
    void place_water_pockets();
    void place_lava_pockets();
    void place_gas_pockets();
    void place_underground_lakes();
    void place_crystal_clusters();

    // ── Bond initialisation ───────────────────────────────────────────────
    void plant_initial_bonds();

    // ── Internal helpers ─────────────────────────────────────────────────
    [[nodiscard]] BiomeZone classify_zone(int wx);
    [[nodiscard]] int       surface_y_at(int wx);
    void carve_circle(int cx, int cy, float radius);
    void fill_circle (int cx, int cy, float radius, ElementID elem);
};

} // namespace pf
