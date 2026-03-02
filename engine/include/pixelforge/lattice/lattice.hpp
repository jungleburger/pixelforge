#pragma once
#include <pixelforge/pixel/settled_pixel.hpp>
#include <pixelforge/core/types.hpp>
#include <unordered_map>
#include <vector>
#include <glm/ext/vector_int2.hpp>

namespace pf {

class Lattice {
public:
    [[nodiscard]] SettledPixel*       get(int wx, int wy);
    [[nodiscard]] const SettledPixel* get(int wx, int wy) const;
    [[nodiscard]] bool                has(int wx, int wy) const;

    void set(int wx, int wy, SettledPixel pixel);
    void remove(int wx, int wy);

    std::vector<std::pair<glm::ivec2, SettledPixel*>>       query_rect(AABB region);
    std::vector<std::pair<glm::ivec2, const SettledPixel*>> query_rect(AABB region) const;

    void   clear();
    [[nodiscard]] size_t size() const { return m_cells.size(); }
    [[nodiscard]] const std::unordered_map<uint64_t, SettledPixel>& cells() const noexcept {
        return m_cells;
    }

private:
    std::unordered_map<uint64_t, SettledPixel> m_cells;

    [[nodiscard]] static uint64_t pack(int wx, int wy) noexcept {
        return (static_cast<uint64_t>(static_cast<uint32_t>(wx)) << 32) |
                static_cast<uint64_t>(static_cast<uint32_t>(wy));
    }
};

} // namespace pf
