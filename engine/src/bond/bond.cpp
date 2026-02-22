#include <pixelforge/bond/bond.hpp>
#include <algorithm>
#include <ranges>

namespace pf {

uint64_t BondManager::create_bond(PixelID pixel_a, PixelID pixel_b,
                                   Direction dir, float strength) {
    const uint64_t id = m_next_id++;
    Bond bond;
    bond.id         = id;
    bond.pixel_a_id = pixel_a;
    bond.pixel_b_id = pixel_b;
    bond.direction  = dir;
    bond.strength   = strength;
    bond.age        = 0.0f;

    m_bonds.emplace(id, bond);
    m_pixel_to_bonds.emplace(pixel_a, id);
    m_pixel_to_bonds.emplace(pixel_b, id);

    return id;
}

void BondManager::break_bond(uint64_t bond_id) {
    auto it = m_bonds.find(bond_id);
    if (it == m_bonds.end()) return;

    const Bond& bond = it->second;

    // Remove from pixel_to_bonds for both pixels
    auto remove_entry = [&](PixelID pid) {
        auto [begin, end] = m_pixel_to_bonds.equal_range(pid);
        for (auto eit = begin; eit != end; ++eit) {
            if (eit->second == bond_id) {
                m_pixel_to_bonds.erase(eit);
                break;
            }
        }
    };
    remove_entry(bond.pixel_a_id);
    remove_entry(bond.pixel_b_id);

    m_bonds.erase(it);
}

void BondManager::break_bonds_for_pixel(PixelID pixel_id) {
    auto [begin, end] = m_pixel_to_bonds.equal_range(pixel_id);

    // Collect bond IDs first to avoid iterator invalidation
    std::vector<uint64_t> bond_ids;
    for (auto it = begin; it != end; ++it) {
        bond_ids.push_back(it->second);
    }

    for (uint64_t bid : bond_ids) {
        break_bond(bid);
    }
}

std::vector<Bond*> BondManager::get_bonds_for_pixel(PixelID pixel_id) {
    std::vector<Bond*> result;
    auto [begin, end] = m_pixel_to_bonds.equal_range(pixel_id);
    for (auto it = begin; it != end; ++it) {
        auto bit = m_bonds.find(it->second);
        if (bit != m_bonds.end()) {
            result.push_back(&bit->second);
        }
    }
    return result;
}

std::vector<const Bond*> BondManager::get_bonds_for_pixel(PixelID pixel_id) const {
    std::vector<const Bond*> result;
    auto [begin, end] = m_pixel_to_bonds.equal_range(pixel_id);
    for (auto it = begin; it != end; ++it) {
        auto bit = m_bonds.find(it->second);
        if (bit != m_bonds.end()) {
            result.push_back(&bit->second);
        }
    }
    return result;
}

int BondManager::bond_count_for_pixel(PixelID pixel_id) const {
    return static_cast<int>(m_pixel_to_bonds.count(pixel_id));
}

void BondManager::tick_age(float dt) {
    for (auto& [id, bond] : m_bonds) {
        bond.age += dt;
    }
}

} // namespace pf
