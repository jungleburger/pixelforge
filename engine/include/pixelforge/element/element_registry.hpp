#pragma once
#include <pixelforge/element/element_def.hpp>
#include <unordered_map>
#include <string>
#include <optional>
#include <vector>
#include <expected>

namespace pf {

enum class RegistryError {
    NotFound,
    DuplicateName,
    InvalidDef
};

class ElementRegistry {
public:
    [[nodiscard]] std::expected<ElementID, RegistryError>
        register_element(ElementDef def);

    [[nodiscard]] std::optional<const ElementDef*>
        find_by_id(ElementID id) const;

    [[nodiscard]] std::optional<const ElementDef*>
        find_by_name(const std::string& name) const;

    [[nodiscard]] const std::vector<ElementDef>& all() const { return m_elements; }

    [[nodiscard]] bool can_bond(ElementID a, ElementID b) const;

    void clear();

    [[nodiscard]] size_t count() const { return m_elements.size(); }

private:
    std::vector<ElementDef>                  m_elements;
    std::unordered_map<ElementID, size_t>    m_by_id;
    std::unordered_map<std::string, size_t>  m_by_name;
    ElementID m_next_id = 1;
};

} // namespace pf
