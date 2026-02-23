#pragma once
#include <pixelforge/core/types.hpp>
#include <cstdint>
#include <glm/vec2.hpp>

namespace pf {

using ElementID = uint16_t;

struct SettledPixel {
    PixelID    id{0};                  // stable unique ID (0 = empty)
    ElementID  element{0};
    float      temperature{20.f};      // degrees C; range [-273, 9999]
    uint8_t    hp{255};                // structural integrity
    uint32_t   color{0xFF'FF'FF'FF};   // RGBA tint override (0 = use element default)
    glm::ivec2 world_pos{0, 0};
};

} // namespace pf
