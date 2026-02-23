#pragma once
#include <cstdint>
#include <algorithm>

namespace pf {

using PixelID   = uint64_t;

constexpr PixelID INVALID_PIXEL_ID     = 0;
constexpr uint16_t INVALID_ELEMENT_ID  = 0;

struct AABB {
    int x{0}, y{0};
    int w{0}, h{0};

    [[nodiscard]] int right()  const noexcept { return x + w; }
    [[nodiscard]] int bottom() const noexcept { return y + h; }
    [[nodiscard]] bool contains(int px, int py) const noexcept {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
    [[nodiscard]] bool empty() const noexcept { return w <= 0 || h <= 0; }

    void expand(int px, int py) noexcept {
        if (empty()) { x = px; y = py; w = 1; h = 1; return; }
        const int nx2 = std::max(x + w, px + 1);
        const int ny2 = std::max(y + h, py + 1);
        x = std::min(x, px);
        y = std::min(y, py);
        w = nx2 - x;
        h = ny2 - y;
    }
};

} // namespace pf
