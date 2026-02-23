#include <pixelforge/renderer/settled_layer.hpp>
#include <pixelforge/core/logger.hpp>

// GL headers — only included inside engine/src/renderer/
#include <glad/glad.h>

namespace pf {

namespace {

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

    glGenTextures(1, &m_tex);
    glBindTexture(GL_TEXTURE_2D, m_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, world_w, world_h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Fullscreen quad
    const float verts[] = {
        // pos         uv
        0.f, 0.f,      0.f, 1.f,
        (float)world_w, 0.f,      1.f, 1.f,
        (float)world_w, (float)world_h, 1.f, 0.f,
        0.f, (float)world_h,      0.f, 0.f,
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

void SettledLayer::upload(const World& world) {
    bool any_dirty = false;
    for (int wy = 0; wy < m_world_h; ++wy) {
        for (int wx = 0; wx < m_world_w; ++wx) {
            const SettledPixel* sp = world.lattice().get(wx, wy);
            uint32_t color = 0x00000000u;
            if (sp) {
                color = sp->color ? sp->color :
                    (world.registry().get(sp->element) ?
                     world.registry().get(sp->element)->color : 0xFFFFFFFFu);
            }
            auto& slot = m_pixel_buf[static_cast<size_t>(wy) * m_world_w + wx];
            if (slot != color) { slot = color; any_dirty = true; }
        }
    }

    if (any_dirty) {
        glBindTexture(GL_TEXTURE_2D, m_tex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_world_w, m_world_h,
                        GL_RGBA, GL_UNSIGNED_BYTE, m_pixel_buf.data());
    }
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
