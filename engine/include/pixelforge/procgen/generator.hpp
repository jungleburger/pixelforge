#pragma once
#include <pixelforge/world/world.hpp>
#include <pixelforge/procgen/biome.hpp>
#include <pixelforge/procgen/noise_utils.hpp>
#include "FastNoiseLite/FastNoiseLite.h"

namespace pf {

struct GeneratorConfig {
    NoiseParams terrain_noise;
    NoiseParams cave_noise;
    NoiseParams ore_noise;
};

class WorldGenerator {
public:
    WorldGenerator(World& world, BiomeRegistry& biomes,
                   GeneratorConfig cfg = {});

    // Fill the whole world with procedurally generated content.
    void generate();

private:
    World&           m_world;
    BiomeRegistry&   m_biomes;
    GeneratorConfig  m_cfg;
    FastNoiseLite    m_terrain_fnl;
    FastNoiseLite    m_cave_fnl;
    FastNoiseLite    m_ore_fnl;

    void generate_terrain_column(int wx);
    void plant_initial_bonds();
};

} // namespace pf
