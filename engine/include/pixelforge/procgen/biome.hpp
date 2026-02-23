#pragma once
#include <pixelforge/core/types.hpp>
#include <pixelforge/element/element_registry.hpp>
#include <string>
#include <vector>

namespace pf {

struct BiomeDef {
    std::string name;
    float       min_depth{0.f};   // normalised 0–1 (0 = surface, 1 = deepest)
    float       max_depth{1.f};
    ElementID   surface_element{0};
    ElementID   fill_element{0};
    ElementID   deep_element{0};
    float       cave_density{0.3f};
    float       ore_chance{0.05f};
    ElementID   ore_element{0};
};

class BiomeRegistry {
public:
    void register_biome(BiomeDef def);
    [[nodiscard]] const BiomeDef* get_biome_at(float depth_norm) const;
    [[nodiscard]] size_t          size() const { return m_biomes.size(); }

private:
    std::vector<BiomeDef> m_biomes;
};

void register_default_biomes(BiomeRegistry& reg, const ElementRegistry& elems);

} // namespace pf
