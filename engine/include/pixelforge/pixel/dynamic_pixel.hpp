#pragma once
#include <pixelforge/core/types.hpp>
#include <glm/vec2.hpp>
#include <cstdint>

namespace pf {

struct DynamicPixel {
    PixelID   id               = INVALID_PIXEL;
    glm::vec2 position         = {0.0f, 0.0f};
    glm::vec2 velocity         = {0.0f, 0.0f};
    float     angle            = 0.0f;
    float     angular_velocity = 0.0f;
    ElementID element          = INVALID_ELEMENT;
    uint32_t  colour           = 0xFFFFFFFF;
    float     temperature      = 293.0f;
    bool      settling         = false;
    uint32_t  frame_born       = 0;
};

} // namespace pf
