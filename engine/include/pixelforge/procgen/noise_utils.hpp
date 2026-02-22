#pragma once
#include <cstdint>
#include <string_view>

// FastNoiseLite – vendored single-header
#include "FastNoiseLite/FastNoiseLite.h"

namespace pf {

struct NoiseParams {
    FastNoiseLite::NoiseType noise_type{FastNoiseLite::NoiseType_OpenSimplex2};
    float frequency{0.01f};
    int   octaves{4};
    float lacunarity{2.0f};
    float gain{0.5f};
};

[[nodiscard]] float sample_noise(FastNoiseLite& fnl, float x, float y) noexcept;
[[nodiscard]] float octave_noise(FastNoiseLite& fnl, float x, float y,
                                 int octaves, float lacunarity, float gain) noexcept;

} // namespace pf
