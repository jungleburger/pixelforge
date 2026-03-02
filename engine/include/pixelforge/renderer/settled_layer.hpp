#pragma once
#include <pixelforge/world/world.hpp>
#include <cstdint>
#include <unordered_set>
#include <vector>

namespace pf {

// Uploads settled pixel data to a GL texture and draws a fullscreen quad.
class SettledLayer {
public:
    ~SettledLayer();

    void init(int world_w, int world_h);
    void shutdown();

    // Upload dirty chunks from world into the texture.
    void upload(const World& world);

    // Enable/disable emissive light propagation pass.
    void set_lighting_enabled(bool enabled) { m_lighting_enabled = enabled; }
    [[nodiscard]] bool lighting_enabled() const noexcept { return m_lighting_enabled; }

    // Optional solid occlusion attenuation for cave-style lighting.
    void set_occlusion_enabled(bool enabled) { m_occlusion_enabled = enabled; }
    [[nodiscard]] bool occlusion_enabled() const noexcept { return m_occlusion_enabled; }
    void set_occlusion_strength(float per_tile) { m_occlusion_strength = per_tile; }
    [[nodiscard]] float occlusion_strength() const noexcept { return m_occlusion_strength; }

    // Draw using the provided MVP (ortho) matrix — 16 floats, column-major.
    void draw(const float* mvp);

private:
    uint32_t m_tex{0};
    uint32_t m_vao{0};
    uint32_t m_vbo{0};
    uint32_t m_shader{0};
    int      m_world_w{0};
    int      m_world_h{0};
    bool     m_lighting_enabled{true};
    bool     m_occlusion_enabled{true};
    float    m_occlusion_strength{0.35f};
    std::vector<uint32_t> m_pixel_buf;
    std::vector<float>    m_light_r;
    std::vector<float>    m_light_g;
    std::vector<float>    m_light_b;
    std::unordered_set<size_t> m_prev_occupied;
    std::unordered_set<size_t> m_curr_occupied;
    std::unordered_set<size_t> m_prev_lit;
    std::unordered_set<size_t> m_curr_lit;
    int m_tile_size{64};
    int m_tiles_x{0};
    int m_tiles_y{0};
    int m_prev_dirty_count{0};
    std::vector<uint8_t> m_dirty_tiles;

    void rebuild_tile_grid(int new_tile_size);
};

} // namespace pf
