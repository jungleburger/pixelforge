#pragma once
#include <cstdint>
#include <glm/vec2.hpp>

namespace pf {

using ElementID = uint16_t;

struct SettledPixel {
    ElementID  element{0};
    uint8_t    temperature{25};        // degrees C, saturated at 255
    uint8_t    hp{255};                // structural integrity
    uint32_t   color{0xFF'FF'FF'FF};   // RGBA tint override (0 = use element default)
    glm::ivec2 world_pos{0, 0};
};

} // namespace pf
