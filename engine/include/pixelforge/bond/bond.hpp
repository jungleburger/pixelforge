#pragma once
#include <pixelforge/core/types.hpp>
#include <cstdint>
#include <vector>
#include <unordered_map>

namespace pf {

enum class Direction : uint8_t {
    North = 0,
    South = 1,
    East  = 2,
    West  = 3
};

struct Bond {
    uint64_t  id{0};
    PixelID   pixel_a_id{0};
    PixelID   pixel_b_id{0};
    Direction direction{Direction::North};
    float     strength{1.f};
    float     age{0.f};
};

class BondManager {
public:
    [[nodiscard]] uint64_t create_bond(PixelID pixel_a, PixelID pixel_b,
                                       Direction dir, float strength = 1.0f);

    void break_bond(uint64_t bond_id);
    void break_bonds_for_pixel(PixelID pixel_id);

    [[nodiscard]] std::vector<Bond*>       get_bonds_for_pixel(PixelID pixel_id);
    [[nodiscard]] std::vector<const Bond*> get_bonds_for_pixel(PixelID pixel_id) const;

    [[nodiscard]] const Bond* get(uint64_t bond_id) const;
    [[nodiscard]] size_t      size() const { return m_bonds.size(); }
    [[nodiscard]] int         bond_count_for_pixel(PixelID pixel_id) const;

    void tick_age(float dt);

private:
    uint64_t m_next_id{1};
    std::unordered_map<uint64_t, Bond>          m_bonds;
    std::unordered_multimap<PixelID, uint64_t>  m_pixel_to_bonds;
};

} // namespace pf
