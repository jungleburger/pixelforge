#pragma once
#include <pixelforge/world/world.hpp>
#include <cstdint>
#include <vector>

namespace pf {

struct DynamicVertex {
    float x, y;
    uint32_t color;
};

// Draws dynamic (in-flight) pixels as point sprites.
class DynamicLayer {
public:
    ~DynamicLayer();

    void init();
    void shutdown();

    // Re-upload current dynamic pixel positions from world.
    void upload(World& world);

    // Draw using the provided MVP matrix — 16 floats, column-major.
    void draw(const float* mvp);

private:
    uint32_t m_vao{0};
    uint32_t m_vbo{0};
    uint32_t m_shader{0};
    uint32_t m_vertex_count{0};
    std::vector<DynamicVertex> m_verts;
};

} // namespace pf
