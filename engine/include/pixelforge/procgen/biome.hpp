#pragma once
#include <pixelforge/core/types.hpp>
#include <pixelforge/element/element_registry.hpp>
#include <string>
#include <vector>

namespace pf {

/// Horizontal climate zone assigned to a terrain column by biome noise.
enum class BiomeZone : uint8_t {
    Temperate = 0,  ///< Default mid-latitude — dirt surface, stone fill
    Cold      = 1,  ///< Polar / icy — ice surface, stone fill
    Arid      = 2,  ///< Desert / volcanic — sand surface, stone fill
};

struct BiomeDef {
    std::string name;
    BiomeZone   zone{BiomeZone::Temperate}; ///< Horizontal climate zone
    float       min_depth{0.f};   // normalised 0–1 (0 = surface, 1 = deepest)
    float       max_depth{1.f};
    ElementID   surface_element{0};  ///< Top-most row of terrain
    ElementID   dirt_element{0};     ///< Subsurface loose-material band (~6 % depth)
    ElementID   fill_element{0};     ///< Bulk stone band
    ElementID   deep_element{0};     ///< Deepest band (bedrock / lava)
    float       cave_density{0.3f};
    float       ore_chance{0.05f};
    ElementID   ore_element{0};
};

class BiomeRegistry {
public:
    void register_biome(BiomeDef def);

    /// Depth-only lookup — returns the first biome (any zone) that spans depth_norm.
    [[nodiscard]] const BiomeDef* get_biome_at(float depth_norm) const;

    /// Zone + depth lookup — prefers matching zone; falls back to any zone.
    [[nodiscard]] const BiomeDef* get_biome_at(float depth_norm, BiomeZone zone) const;

    [[nodiscard]] size_t size() const { return m_biomes.size(); }

private:
    std::vector<BiomeDef> m_biomes;
};

void register_default_biomes(BiomeRegistry& reg, const ElementRegistry& elems);

} // namespace pf
