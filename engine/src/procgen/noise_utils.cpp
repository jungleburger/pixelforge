#include <pixelforge/procgen/noise_utils.hpp>

namespace pf {

float sample_noise(FastNoiseLite& fnl, float x, float y) noexcept {
    // Returns value in [-1, 1]
    return fnl.GetNoise(x, y);
}

float octave_noise(FastNoiseLite& fnl, float x, float y,
                   int octaves, float lacunarity, float gain) noexcept {
    float value     = 0.f;
    float amplitude = 1.f;
    float frequency = 1.f;
    float max_value = 0.f;

    for (int i = 0; i < octaves; ++i) {
        value     += fnl.GetNoise(x * frequency, y * frequency) * amplitude;
        max_value += amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }

    return (max_value > 0.f) ? value / max_value : 0.f;
}

} // namespace pf
