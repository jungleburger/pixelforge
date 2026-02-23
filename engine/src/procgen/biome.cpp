#include <pixelforge/procgen/biome.hpp>
#include <algorithm>

namespace pf {

void BiomeRegistry::register_biome(BiomeDef def) {
    m_biomes.push_back(std::move(def));
    // Keep sorted by min_depth ascending for quick lookup.
    std::sort(m_biomes.begin(), m_biomes.end(),
        [](const BiomeDef& a, const BiomeDef& b) {
            return a.min_depth < b.min_depth;
        });
}

const BiomeDef* BiomeRegistry::get_biome_at(float depth_norm) const {
    const BiomeDef* best = nullptr;
    for (const auto& b : m_biomes) {
        if (depth_norm >= b.min_depth && depth_norm <= b.max_depth) {
            best = &b;
            break;
        }
    }
    return best;
}

void register_default_biomes(BiomeRegistry& reg, const ElementRegistry& elems) {
    // Surface biome
    {
        BiomeDef b;
        b.name            = "Surface";
        b.min_depth       = 0.f;
        b.max_depth       = 0.2f;
        b.surface_element = elems.id_of("sand");
        b.fill_element    = elems.id_of("stone");
        b.deep_element    = elems.id_of("stone");
        b.cave_density    = 0.15f;
        reg.register_biome(b);
    }
    // Mid biome
    {
        BiomeDef b;
        b.name            = "Underground";
        b.min_depth       = 0.2f;
        b.max_depth       = 0.75f;
        b.surface_element = elems.id_of("stone");
        b.fill_element    = elems.id_of("stone");
        b.deep_element    = elems.id_of("stone");
        b.cave_density    = 0.35f;
        b.ore_chance      = 0.04f;
        reg.register_biome(b);
    }
    // Deep biome
    {
        BiomeDef b;
        b.name            = "Deep";
        b.min_depth       = 0.75f;
        b.max_depth       = 1.f;
        b.surface_element = elems.id_of("stone");
        b.fill_element    = elems.id_of("stone");
        b.deep_element    = elems.id_of("lava");
        b.cave_density    = 0.45f;
        reg.register_biome(b);
    }
}

} // namespace pf
