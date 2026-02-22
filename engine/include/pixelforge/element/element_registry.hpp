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
    float        melting_point{-1.f};   // -1 = doesn't melt
    float        boiling_point{-1.f};   // -1 = doesn't evaporate
    ElementID    melt_into{0};
    ElementID    boil_into{0};
    bool         emits_light{false};
    float        light_radius{0.f};
    uint32_t     light_color{0xFF'FF'FF'FF};
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
