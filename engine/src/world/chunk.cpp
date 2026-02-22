#include <pixelforge/world/chunk.hpp>

namespace pf {

bool Chunk::has_cell(int lx, int ly) const {
    return cells.contains(cell_key(lx, ly));
}

SettledPixel* Chunk::get_cell(int lx, int ly) {
    auto it = cells.find(cell_key(lx, ly));
    return (it != cells.end()) ? &it->second : nullptr;
}

void Chunk::set_cell(int lx, int ly, SettledPixel pixel) {
    pixel.world_pos = {chunk_x * CHUNK_SIZE + lx,
                       chunk_y * CHUNK_SIZE + ly};
    cells.insert_or_assign(cell_key(lx, ly), std::move(pixel));
    mark_dirty(lx, ly);
}

void Chunk::clear_cell(int lx, int ly) {
    cells.erase(cell_key(lx, ly));
    mark_dirty(lx, ly);
}

void Chunk::mark_dirty(int lx, int ly) {
    dirty = true;
    sleeping = false;
    dirty_region.expand(lx, ly);
}

void Chunk::wake() {
    sleeping = false;
}

void Chunk::try_sleep() {
    if (dynamic_pixels.empty()) {
        sleeping = true;
    }
}

} // namespace pf
