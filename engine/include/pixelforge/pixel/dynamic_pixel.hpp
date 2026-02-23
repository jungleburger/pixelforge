#pragma once
#include <pixelforge/pixel/settled_pixel.hpp>
#include <glm/vec2.hpp>

namespace pf {

struct DynamicPixel {
    SettledPixel base;
    PixelID      id{0};
    glm::vec2    pos{0.f, 0.f};   // sub-pixel world position
    glm::vec2    vel{0.f, 0.f};   // pixels / second
    float        lifetime{-1.f};  // -1 = immortal
    float        age{0.f};
    bool         awake{true};
    bool         pending_settle{false};
};

} // namespace pf
