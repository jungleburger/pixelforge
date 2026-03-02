#include <pixelforge/renderer/settled_layer.hpp>
#include <pixelforge/core/logger.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

// GL headers — only included inside engine/src/renderer/
#include <glad/glad.h>

namespace pf {

namespace {

// Adaptive tile sizes — chosen per frame based on dirty density.
constexpr int TILE_SMALL  =  32;
constexpr int TILE_MEDIUM =  64;
constexpr int TILE_LARGE  = 128;

static int choose_tile_size(int prev_dirty_tiles, int total_tiles) {
    if (total_tiles <= 0) return TILE_MEDIUM;
    const float density = static_cast<float>(prev_dirty_tiles) / static_cast<float>(total_tiles);
    if (density > 0.40f) return TILE_LARGE;   // many changes → fewer API calls
    if (density < 0.05f) return TILE_SMALL;   // sparse → less wasted bandwidth
    return TILE_MEDIUM;
}

struct Rgba {
    float r{0.f};
    float g{0.f};
    float b{0.f};
    float a{0.f};
};

static Rgba unpack_rgba(uint32_t c) {
    return {
        static_cast<float>((c >> 24) & 0xFFu),
        static_cast<float>((c >> 16) & 0xFFu),
        static_cast<float>((c >>  8) & 0xFFu),
        static_cast<float>( c        & 0xFFu)
    };
}

static uint32_t pack_rgba(float r, float g, float b, float a) {
    const auto to_u8 = [](float v) -> uint32_t {
        return static_cast<uint32_t>(std::clamp(std::lround(v), 0l, 255l));
    };

    return (to_u8(r) << 24) |
           (to_u8(g) << 16) |
           (to_u8(b) <<  8) |
            to_u8(a);
}

struct Emitter {
    int   x{0};
    int   y{0};
    float radius{0.f};
    Rgba  color;
};

static float occlusion_transmittance(const World& world,
                                     int x0, int y0,
                                     int x1, int y1,
                                     float occlusion_strength) {
    int x = x0;
    int y = y0;

    const int dx = std::abs(x1 - x0);
    const int sx = (x0 < x1) ? 1 : -1;
    const int dy = -std::abs(y1 - y0);
    const int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    float transmittance = 1.f;
    int step_counter = 0;
    constexpr int CHECK_INTERVAL = 2;

    while (x != x1 || y != y1) {
        const int e2 = err * 2;
        if (e2 >= dy) { err += dy; x += sx; }
        if (e2 <= dx) { err += dx; y += sy; }

        if (x == x1 && y == y1) break;

        if ((++step_counter % CHECK_INTERVAL) == 0 && world.lattice().has(x, y)) {
            transmittance *= (1.f - std::clamp(occlusion_strength, 0.f, 0.95f));
            if (transmittance <= 0.01f) return 0.f;
        }
    }

    return transmittance;
}

const char* VERT_SRC = R"(
#version 430 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
out vec2 vUV;
uniform mat4 uMVP;
void main() {
    vUV = aUV;
    gl_Position = uMVP * vec4(aPos, 0.0, 1.0);
}
)";

const char* FRAG_SRC = R"(
#version 430 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uTex;
void main() {
    FragColor = texture(uTex, vUV);
}
)";

uint32_t compile_shader(GLenum type, const char* src) {
    uint32_t s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetShaderInfoLog(s, sizeof(buf), nullptr, buf);
        PF_LOG_ERROR("Shader compile error: {}", buf);
    }
    return s;
}

uint32_t link_program(uint32_t vert, uint32_t frag) {
    uint32_t p = glCreateProgram();
    glAttachShader(p, vert);
    glAttachShader(p, frag);
    glLinkProgram(p);
    int ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetProgramInfoLog(p, sizeof(buf), nullptr, buf);
        PF_LOG_ERROR("Program link error: {}", buf);
    }
    return p;
}

} // anonymous namespace

SettledLayer::~SettledLayer() { shutdown(); }

void SettledLayer::init(int world_w, int world_h) {
    m_world_w = world_w;
    m_world_h = world_h;
    m_pixel_buf.assign(static_cast<size_t>(world_w) * world_h, 0u);
    m_light_r.assign(static_cast<size_t>(world_w) * world_h, 0.f);
    m_light_g.assign(static_cast<size_t>(world_w) * world_h, 0.f);
    m_light_b.assign(static_cast<size_t>(world_w) * world_h, 0.f);
    m_prev_occupied.clear();
    m_curr_occupied.clear();
    m_prev_lit.clear();
    m_curr_lit.clear();
    m_prev_dirty_count = 0;
    rebuild_tile_grid(TILE_MEDIUM);

    glGenTextures(1, &m_tex);
    glBindTexture(GL_TEXTURE_2D, m_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, world_w, world_h, 0,
                 GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, nullptr);

    // Fullscreen quad
    const float verts[] = {
        // pos         uv
        0.f, 0.f,      0.f, 0.f,
        (float)world_w, 0.f,      1.f, 0.f,
        (float)world_w, (float)world_h, 1.f, 1.f,
        0.f, (float)world_h,      0.f, 1.f,
    };
    const uint32_t idx[] = {0,1,2, 0,2,3};

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    uint32_t ebo = 0;
    glGenBuffers(1, &ebo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    auto vert = compile_shader(GL_VERTEX_SHADER, VERT_SRC);
    auto frag = compile_shader(GL_FRAGMENT_SHADER, FRAG_SRC);
    m_shader  = link_program(vert, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);
}

void SettledLayer::shutdown() {
    if (m_shader) { glDeleteProgram(m_shader); m_shader = 0; }
    if (m_vao)    { glDeleteVertexArrays(1, &m_vao); m_vao = 0; }
    if (m_vbo)    { glDeleteBuffers(1, &m_vbo); m_vbo = 0; }
    if (m_tex)    { glDeleteTextures(1, &m_tex); m_tex = 0; }
}

void SettledLayer::rebuild_tile_grid(int new_tile_size) {
    m_tile_size = new_tile_size;
    m_tiles_x = (m_world_w + m_tile_size - 1) / m_tile_size;
    m_tiles_y = (m_world_h + m_tile_size - 1) / m_tile_size;
    m_dirty_tiles.assign(static_cast<size_t>(m_tiles_x) * m_tiles_y, 0u);
}

void SettledLayer::upload(const World& world) {
    // Adapt tile size based on previous frame's dirty density.
    const int ideal = choose_tile_size(m_prev_dirty_count, m_tiles_x * m_tiles_y);
    if (ideal != m_tile_size) {
        rebuild_tile_grid(ideal);
    }

    std::vector<Emitter> emitters;
    emitters.reserve(256);

    // Clear only previously lit indices (sparse reset) instead of full-world fills.
    for (const size_t idx : m_prev_lit) {
        m_light_r[idx] = 0.f;
        m_light_g[idx] = 0.f;
        m_light_b[idx] = 0.f;
    }
    m_curr_lit.clear();
    m_curr_lit.reserve(m_prev_lit.size() + 256);

    if (m_lighting_enabled) {
        // Settled emissive pixels
        for (const auto& [key, sp] : world.lattice().cells()) {
            const int wx = sp.world_pos.x;
            const int wy = sp.world_pos.y;
            if (wx < 0 || wx >= m_world_w || wy < 0 || wy >= m_world_h) continue;
            const ElementDef* def = world.registry().get(sp.element);
            if (!def || !def->emits_light || def->light_radius <= 0.f) continue;

            emitters.push_back(Emitter{
                wx,
                wy,
                std::max(1.f, def->light_radius),
                unpack_rgba(def->light_color)
            });
        }

        // Dynamic emissive pixels
        for (const DynamicPixel* dp : world.collect_all_dynamic()) {
            if (!dp || !dp->awake) continue;
            const ElementDef* def = world.registry().get(dp->base.element);
            if (!def || !def->emits_light || def->light_radius <= 0.f) continue;

            const int ex = static_cast<int>(std::lround(dp->pos.x));
            const int ey = static_cast<int>(std::lround(dp->pos.y));
            if (ex < 0 || ex >= m_world_w || ey < 0 || ey >= m_world_h) continue;

            emitters.push_back(Emitter{
                ex,
                ey,
                std::max(1.f, def->light_radius),
                unpack_rgba(def->light_color)
            });
        }

        for (const Emitter& emitter : emitters) {
            const int radius_i = std::max(1, static_cast<int>(std::ceil(emitter.radius)));
            const int min_x = std::max(0, emitter.x - radius_i);
            const int max_x = std::min(m_world_w - 1, emitter.x + radius_i);
            const int min_y = std::max(0, emitter.y - radius_i);
            const int max_y = std::min(m_world_h - 1, emitter.y + radius_i);
            const float inv_radius = 1.f / emitter.radius;

            for (int y = min_y; y <= max_y; ++y) {
                for (int x = min_x; x <= max_x; ++x) {
                    const float dx = static_cast<float>(x - emitter.x);
                    const float dy = static_cast<float>(y - emitter.y);
                    const float distance = std::sqrt(dx * dx + dy * dy);
                    if (distance > emitter.radius) continue;

                    const float linear = 1.f - (distance * inv_radius);
                    float intensity = linear * linear;

                    if (m_occlusion_enabled) {
                        intensity *= occlusion_transmittance(
                            world,
                            emitter.x, emitter.y,
                            x, y,
                            m_occlusion_strength);
                        if (intensity <= 0.f) continue;
                    }

                    const size_t idx = static_cast<size_t>(y) * m_world_w + x;
                    m_light_r[idx] += (emitter.color.r / 255.f) * intensity;
                    m_light_g[idx] += (emitter.color.g / 255.f) * intensity;
                    m_light_b[idx] += (emitter.color.b / 255.f) * intensity;
                    m_curr_lit.insert(idx);
                }
            }
        }
    }

    std::swap(m_prev_lit, m_curr_lit);

    auto compose_pixel = [&](const SettledPixel& sp, int wx, int wy) -> uint32_t {
        uint32_t color = sp.color ? sp.color :
            (world.registry().get(sp.element) ?
             world.registry().get(sp.element)->color : 0xFFFFFFFFu);

        if (m_lighting_enabled) {
            const size_t idx = static_cast<size_t>(wy) * m_world_w + wx;
            const float ambient = 0.35f;
            const float lr = std::clamp(m_light_r[idx], 0.f, 1.f);
            const float lg = std::clamp(m_light_g[idx], 0.f, 1.f);
            const float lb = std::clamp(m_light_b[idx], 0.f, 1.f);
            const Rgba base = unpack_rgba(color);

            const float mod_r = std::max(ambient, lr);
            const float mod_g = std::max(ambient, lg);
            const float mod_b = std::max(ambient, lb);

            color = pack_rgba(base.r * mod_r,
                              base.g * mod_g,
                              base.b * mod_b,
                              base.a);
        }
        return color;
    };

    bool any_dirty = false;
    std::fill(m_dirty_tiles.begin(), m_dirty_tiles.end(), 0u);

    const auto mark_dirty = [&](size_t idx) {
        const int y = static_cast<int>(idx / static_cast<size_t>(m_world_w));
        const int x = static_cast<int>(idx % static_cast<size_t>(m_world_w));
        const int tx = x / m_tile_size;
        const int ty = y / m_tile_size;
        m_dirty_tiles[static_cast<size_t>(ty) * m_tiles_x + tx] = 1u;
    };

    m_curr_occupied.clear();
    m_curr_occupied.reserve(world.lattice().size());

    for (const auto& [key, sp] : world.lattice().cells()) {
        const int wx = sp.world_pos.x;
        const int wy = sp.world_pos.y;
        if (wx < 0 || wx >= m_world_w || wy < 0 || wy >= m_world_h) continue;

        const size_t idx = static_cast<size_t>(wy) * m_world_w + wx;
        m_curr_occupied.insert(idx);
        const uint32_t color = compose_pixel(sp, wx, wy);
        auto& slot = m_pixel_buf[idx];
        if (slot != color) {
            slot = color;
            any_dirty = true;
            mark_dirty(idx);
        }
    }

    for (const size_t idx : m_prev_occupied) {
        if (m_curr_occupied.contains(idx)) continue;
        auto& slot = m_pixel_buf[idx];
        if (slot != 0x00000000u) {
            slot = 0x00000000u;
            any_dirty = true;
            mark_dirty(idx);
        }
    }

    std::swap(m_prev_occupied, m_curr_occupied);

    if (any_dirty) {
        glBindTexture(GL_TEXTURE_2D, m_tex);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, m_world_w);

        for (int ty = 0; ty < m_tiles_y; ++ty) {
            for (int tx = 0; tx < m_tiles_x; ++tx) {
                if (!m_dirty_tiles[static_cast<size_t>(ty) * m_tiles_x + tx]) continue;

                const int upload_x = tx * m_tile_size;
                const int upload_y = ty * m_tile_size;
                const int upload_w = std::min(m_tile_size, m_world_w - upload_x);
                const int upload_h = std::min(m_tile_size, m_world_h - upload_y);

                const uint8_t* src = reinterpret_cast<const uint8_t*>(m_pixel_buf.data()) +
                    (static_cast<size_t>(upload_y) * m_world_w + upload_x) * sizeof(uint32_t);

                glTexSubImage2D(GL_TEXTURE_2D,
                                0,
                                upload_x,
                                upload_y,
                                upload_w,
                                upload_h,
                                GL_RGBA,
                                GL_UNSIGNED_INT_8_8_8_8,
                                src);
            }
        }

        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    }

    // Record dirty tile count so next frame can adapt tile size.
    m_prev_dirty_count = 0;
    for (const auto d : m_dirty_tiles) m_prev_dirty_count += (d != 0);
}

void SettledLayer::draw(const float* mvp) {
    glUseProgram(m_shader);
    glUniformMatrix4fv(glGetUniformLocation(m_shader, "uMVP"), 1, GL_FALSE, mvp);
    glBindTexture(GL_TEXTURE_2D, m_tex);
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

} // namespace pf
