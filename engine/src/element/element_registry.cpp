#include <pixelforge/element/element_registry.hpp>
#include <pixelforge/core/logger.hpp>
#include <stdexcept>
#include <format>

namespace pf {

ElementRegistry::ElementRegistry() {
    // Reserve slot 0 as "air / invalid"
    ElementDef air;
    air.id      = 0;
    air.name    = "Air";
    air.tag     = "air";
    air.physics = PhysicsModel::Gas;
    air.color   = 0x00000000u;
    air.density = 0.f;
    m_defs.push_back(air);
    m_tag_index["air"] = 0;
}

ElementID ElementRegistry::register_element(ElementDef def) {
    if (m_tag_index.contains(def.tag)) {
        throw std::runtime_error(
            std::format("ElementRegistry: duplicate tag '{}'", def.tag));
    }
    const ElementID id = static_cast<ElementID>(m_defs.size());
    def.id = id;
    m_tag_index[def.tag] = id;
    m_defs.push_back(std::move(def));
    return id;
}

const ElementDef* ElementRegistry::get(ElementID id) const {
    if (id >= m_defs.size()) return nullptr;
    return &m_defs[id];
}

const ElementDef* ElementRegistry::get(std::string_view tag) const {
    auto it = m_tag_index.find(std::string{tag});
    if (it == m_tag_index.end()) return nullptr;
    return &m_defs[it->second];
}

ElementID ElementRegistry::id_of(std::string_view tag) const {
    auto it = m_tag_index.find(std::string{tag});
    return (it != m_tag_index.end()) ? it->second : INVALID_ELEMENT_ID;
}

} // namespace pf
