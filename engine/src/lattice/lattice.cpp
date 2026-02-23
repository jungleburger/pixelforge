#include <pixelforge/lattice/lattice.hpp>

namespace pf {

SettledPixel* Lattice::get(int wx, int wy) {
    auto it = m_cells.find(pack(wx, wy));
    return (it != m_cells.end()) ? &it->second : nullptr;
}

const SettledPixel* Lattice::get(int wx, int wy) const {
    auto it = m_cells.find(pack(wx, wy));
    return (it != m_cells.end()) ? &it->second : nullptr;
}

bool Lattice::has(int wx, int wy) const {
    return m_cells.contains(pack(wx, wy));
}

void Lattice::set(int wx, int wy, SettledPixel pixel) {
    pixel.world_pos = {wx, wy};
    m_cells.insert_or_assign(pack(wx, wy), std::move(pixel));
}

void Lattice::remove(int wx, int wy) {
    m_cells.erase(pack(wx, wy));
}

std::vector<std::pair<glm::ivec2, SettledPixel*>>
Lattice::query_rect(AABB region) {
    std::vector<std::pair<glm::ivec2, SettledPixel*>> result;
    for (int y = region.y; y < region.bottom(); ++y) {
        for (int x = region.x; x < region.right(); ++x) {
            auto it = m_cells.find(pack(x, y));
            if (it != m_cells.end()) {
                result.emplace_back(glm::ivec2{x, y}, &it->second);
            }
        }
    }
    return result;
}

std::vector<std::pair<glm::ivec2, const SettledPixel*>>
Lattice::query_rect(AABB region) const {
    std::vector<std::pair<glm::ivec2, const SettledPixel*>> result;
    for (int y = region.y; y < region.bottom(); ++y) {
        for (int x = region.x; x < region.right(); ++x) {
            auto it = m_cells.find(pack(x, y));
            if (it != m_cells.end()) {
                result.emplace_back(glm::ivec2{x, y}, &it->second);
            }
        }
    }
    return result;
}

void Lattice::clear() {
    m_cells.clear();
}

} // namespace pf
