#pragma once
#include <pixelforge/pixel/settled_pixel.hpp>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace pf {

enum class PhysicsModel : uint8_t {
    Solid,
    Powder,
    Liquid,
    Gas,
    Fire,
    Plasma,
    Energy
};

struct ElementDef {
    ElementID    id{0};
    std::string  name;
    std::string  tag;              // short lowercase key, e.g. "sand"
    PhysicsModel physics{PhysicsModel::Solid};
    uint32_t     color{0xFF'AA'88'FF};  // RGBA default colour
    float        density{1.f};
    float        viscosity{0.f};        // 0 = free-flowing, 1 = fully viscous
    float        flammability{0.f};     // 0 = non-flammable, 1 = instantly ignites
    float        restitution{0.35f};    // bounciness (0 = dead stop, 1 = perfect bounce)
    float        melting_point{-1.f};   // -1 = doesn't melt
    float        boiling_point{-1.f};   // -1 = doesn't evaporate
    ElementID    melt_into{0};
    ElementID    boil_into{0};
    bool         emits_light{false};
    float        light_radius{0.f};
    uint32_t     light_color{0xFF'FF'FF'FF};

    // --- Phase 3: Reaction fields -----------------------------------------
    float        ignition_point{-1.f};       // temp (°C) at which element catches fire; -1 = never
    float        thermal_conductivity{0.05f};// fraction of temp delta transferred per second (0..1)
    float        heat_output{0.f};           // heat (°C/s) emitted by Fire/Liquid elements

    // Tag strings for cross-element references (resolved at reaction time)
    std::string  melt_into_tag;   // e.g. "lava" — overrides melt_into ID when non-empty
    std::string  boil_into_tag;   // e.g. "steam"
    std::string  ash_into_tag;    // e.g. "stone" — what a burnt element leaves behind

    // --- Phase 4: Solidification / condensation ---------------------------
    // When temperature drops AT OR BELOW solidify_point the pixel converts.
    // Examples: lava (800 °C → stone), steam (100 °C → water), water (0 °C → ice)
    float        solidify_point{-1.f};       // -1 = never solidifies
    std::string  solidify_into_tag;          // element tag to become on solidification

    // --- Phase 4: Contact reactions ----------------------------------------
    // Checked each tick for every settled pixel against its 4 cardinal neighbours.
    struct ContactReaction {
        std::string  target_tag;         // neighbour element that triggers this reaction
        std::string  self_into_tag;      // what THIS pixel becomes  (empty = no change)
        std::string  other_into_tag;     // what the OTHER pixel becomes (empty = disappear)
        float        probability{1.f};   // per-second chance of reaction (0..1)
    };
    std::vector<ContactReaction> reactions;  ///< contact-based reactions with neighbours
};

class ElementRegistry {
public:
    ElementRegistry();

    ElementID register_element(ElementDef def);

    [[nodiscard]] const ElementDef* get(ElementID id) const;
    [[nodiscard]] const ElementDef* get(std::string_view tag) const;
    [[nodiscard]] ElementID         id_of(std::string_view tag) const;
    [[nodiscard]] size_t            size() const { return m_defs.size(); }

    [[nodiscard]] auto begin() const { return m_defs.begin(); }
    [[nodiscard]] auto end()   const { return m_defs.end();   }

private:
    std::vector<ElementDef>                    m_defs;
    std::unordered_map<std::string, ElementID> m_tag_index;
};

} // namespace pf
