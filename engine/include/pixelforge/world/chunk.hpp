#pragma once
#include <pixelforge/pixel/settled_pixel.hpp>
#include <pixelforge/pixel/dynamic_pixel.hpp>
#include <pixelforge/core/types.hpp>
#include <unordered_map>
#include <vector>

namespace pf {

constexpr int CHUNK_SIZE = 128;

struct Chunk {
    int32_t chunk_x = 0;
    int32_t chunk_y = 0;

    std::unordered_map<uint16_t, SettledPixel> cells;
    std::vector<DynamicPixel> dynamic_pixels;

    bool  sleeping     = false;
    bool  dirty        = true;
    AABB  dirty_region = {};

    [[nodiscard]] bool          has_cell(int lx, int ly) const;
    [[nodiscard]] SettledPixel* get_cell(int lx, int ly);
    void set_cell(int lx, int ly, SettledPixel pixel);
    void clear_cell(int lx, int ly);
    void mark_dirty(int lx, int ly);
    void wake();
    void try_sleep();

    [[nodiscard]] static constexpr uint16_t cell_key(int lx, int ly) noexcept {
        return static_cast<uint16_t>((lx << 7) | ly);
    }
};

} // namespace pf
