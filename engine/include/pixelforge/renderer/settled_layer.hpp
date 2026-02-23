#pragma once
#include <pixelforge/world/world.hpp>
#include <cstdint>
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

    // Draw using the provided MVP (ortho) matrix — 16 floats, column-major.
    void draw(const float* mvp);

private:
    uint32_t m_tex{0};
    uint32_t m_vao{0};
    uint32_t m_vbo{0};
    uint32_t m_shader{0};
    int      m_world_w{0};
    int      m_world_h{0};
    std::vector<uint32_t> m_pixel_buf;
};

} // namespace pf
