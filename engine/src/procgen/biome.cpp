#include <pixelforge/procgen/biome.hpp>
#include <algorithm>

namespace pf {

void BiomeRegistry::register_biome(BiomeDef def) {
    m_biomes.push_back(std::move(def));
    // Keep sorted by (zone, min_depth) for deterministic lookup.
    std::sort(m_biomes.begin(), m_biomes.end(),
        [](const BiomeDef& a, const BiomeDef& b) {
            if (a.zone != b.zone) return a.zone < b.zone;
            return a.min_depth < b.min_depth;
        });
}

const BiomeDef* BiomeRegistry::get_biome_at(float depth_norm) const {
    // Backwards-compat: just zone-agnostic depth search.
    for (const auto& b : m_biomes) {
        if (depth_norm >= b.min_depth && depth_norm <= b.max_depth)
            return &b;
    }
    return nullptr;
}

const BiomeDef* BiomeRegistry::get_biome_at(float depth_norm, BiomeZone zone) const {
    // First pass: exact zone match
    const BiomeDef* best = nullptr;
    for (const auto& b : m_biomes) {
        if (b.zone == zone && depth_norm >= b.min_depth && depth_norm <= b.max_depth) {
            best = &b;
            break;
        }
    }
    if (best) return best;
    // Fallback: depth-only (any zone)
    return get_biome_at(depth_norm);
}

// ─────────────────────────────────────────────────────────────────────────────

void register_default_biomes(BiomeRegistry& reg, const ElementRegistry& elems) {

    // Helper to safely look up an element ID (returns 0 if not registered yet).
    auto E = [&](const char* tag) -> ElementID { return elems.id_of(tag); };

    // ── Temperate (dirt surface → stone → bedrock) ────────────────────────
    {
        BiomeDef b;
        b.name            = "Temperate Surface";
        b.zone            = BiomeZone::Temperate;
        b.min_depth       = 0.f;
        b.max_depth       = 0.2f;
        b.surface_element = E("dirt");
        b.dirt_element    = E("dirt");
        b.fill_element    = E("stone");
        b.deep_element    = E("stone");
        b.cave_density    = 0.12f;
        reg.register_biome(b);
    }
    {
        BiomeDef b;
        b.name            = "Temperate Underground";
        b.zone            = BiomeZone::Temperate;
        b.min_depth       = 0.2f;
        b.max_depth       = 0.75f;
        b.surface_element = E("stone");
        b.dirt_element    = E("stone");
        b.fill_element    = E("stone");
        b.deep_element    = E("stone");
        b.cave_density    = 0.30f;
        b.ore_chance      = 0.04f;
        b.ore_element     = E("crystal");
        reg.register_biome(b);
    }
    {
        BiomeDef b;
        b.name            = "Temperate Deep";
        b.zone            = BiomeZone::Temperate;
        b.min_depth       = 0.75f;
        b.max_depth       = 1.f;
        b.surface_element = E("stone");
        b.dirt_element    = E("stone");
        b.fill_element    = E("stone");
        b.deep_element    = E("lava");
        b.cave_density    = 0.40f;
        b.ore_chance      = 0.025f;
        b.ore_element     = E("crystal");
        reg.register_biome(b);
    }

    // ── Cold (ice/snow surface → stone → frozen deep) ─────────────────────
    {
        BiomeDef b;
        b.name            = "Cold Surface";
        b.zone            = BiomeZone::Cold;
        b.min_depth       = 0.f;
        b.max_depth       = 0.2f;
        b.surface_element = E("ice");
        b.dirt_element    = E("ice");
        b.fill_element    = E("stone");
        b.deep_element    = E("stone");
        b.cave_density    = 0.08f;
        reg.register_biome(b);
    }
    {
        BiomeDef b;
        b.name            = "Cold Underground";
        b.zone            = BiomeZone::Cold;
        b.min_depth       = 0.2f;
        b.max_depth       = 0.8f;
        b.surface_element = E("stone");
        b.dirt_element    = E("stone");
        b.fill_element    = E("stone");
        b.deep_element    = E("stone");
        b.cave_density    = 0.25f;
        b.ore_chance      = 0.03f;
        b.ore_element     = E("crystal");
        reg.register_biome(b);
    }
    {
        BiomeDef b;
        b.name            = "Cold Deep";
        b.zone            = BiomeZone::Cold;
        b.min_depth       = 0.8f;
        b.max_depth       = 1.f;
        b.surface_element = E("stone");
        b.dirt_element    = E("stone");
        b.fill_element    = E("stone");
        b.deep_element    = E("stone");
        b.cave_density    = 0.35f;
        reg.register_biome(b);
    }

    // ── Arid (sand surface → stone → lava deep) ───────────────────────────
    {
        BiomeDef b;
        b.name            = "Arid Surface";
        b.zone            = BiomeZone::Arid;
        b.min_depth       = 0.f;
        b.max_depth       = 0.2f;
        b.surface_element = E("sand");
        b.dirt_element    = E("sand");
        b.fill_element    = E("stone");
        b.deep_element    = E("stone");
        b.cave_density    = 0.18f;
        reg.register_biome(b);
    }
    {
        BiomeDef b;
        b.name            = "Arid Underground";
        b.zone            = BiomeZone::Arid;
        b.min_depth       = 0.2f;
        b.max_depth       = 0.70f;
        b.surface_element = E("stone");
        b.dirt_element    = E("stone");
        b.fill_element    = E("stone");
        b.deep_element    = E("stone");
        b.cave_density    = 0.38f;
        b.ore_chance      = 0.05f;
        b.ore_element     = E("crystal");
        reg.register_biome(b);
    }
    {
        BiomeDef b;
        b.name            = "Arid Deep";
        b.zone            = BiomeZone::Arid;
        b.min_depth       = 0.70f;
        b.max_depth       = 1.f;
        b.surface_element = E("stone");
        b.dirt_element    = E("stone");
        b.fill_element    = E("stone");
        b.deep_element    = E("lava");
        b.cave_density    = 0.48f;
        b.ore_chance      = 0.03f;
        b.ore_element     = E("crystal");
        reg.register_biome(b);
    }
}

} // namespace pf
