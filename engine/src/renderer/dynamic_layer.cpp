#include <pixelforge/renderer/dynamic_layer.hpp>
#include <pixelforge/core/logger.hpp>
#include <glad/glad.h>

namespace pf {

namespace {

const char* VERT_SRC = R"(
#version 430 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aColor;
out vec4 vColor;
uniform mat4 uMVP;
void main() {
    vColor = aColor;
    gl_Position = uMVP * vec4(aPos, 0.0, 1.0);
    gl_PointSize = 2.0;
}
)";

const char* FRAG_SRC = R"(
#version 430 core
in vec4 vColor;
out vec4 FragColor;
void main() { FragColor = vColor; }
)";

uint32_t compile_shader(GLenum type, const char* src) {
    uint32_t s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    return s;
}

uint32_t link_program(uint32_t v, uint32_t f) {
    uint32_t p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f);
    glLinkProgram(p);
    return p;
}

} // anonymous namespace

DynamicLayer::~DynamicLayer() { shutdown(); }

void DynamicLayer::init() {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    // Pos (2 floats) + color (4 bytes packed as float)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(DynamicVertex),
                          (void*)offsetof(DynamicVertex, x));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(DynamicVertex),
                          (void*)offsetof(DynamicVertex, color));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    auto vert = compile_shader(GL_VERTEX_SHADER, VERT_SRC);
    auto frag = compile_shader(GL_FRAGMENT_SHADER, FRAG_SRC);
    m_shader  = link_program(vert, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);
}

void DynamicLayer::shutdown() {
    if (m_shader) { glDeleteProgram(m_shader); m_shader = 0; }
    if (m_vao)    { glDeleteVertexArrays(1, &m_vao); m_vao = 0; }
    if (m_vbo)    { glDeleteBuffers(1, &m_vbo); m_vbo = 0; }
}

void DynamicLayer::upload(World& world) {
    m_verts.clear();

    for (auto* dp : world.collect_all_dynamic()) {
        if (!dp->awake) continue;
        uint32_t color = dp->base.color ? dp->base.color : 0xFFFFFFFFu;
        if (!dp->base.color) {
            const ElementDef* def = world.registry().get(dp->base.element);
            if (def) color = def->color;
        }
        m_verts.push_back({dp->pos.x, dp->pos.y, color});
    }

    m_vertex_count = static_cast<uint32_t>(m_verts.size());

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_verts.size() * sizeof(DynamicVertex)),
                 m_verts.data(), GL_DYNAMIC_DRAW);
}

void DynamicLayer::draw(const float* mvp) {
    if (m_vertex_count == 0) return;
    glUseProgram(m_shader);
    glUniformMatrix4fv(glGetUniformLocation(m_shader, "uMVP"), 1, GL_FALSE, mvp);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glBindVertexArray(m_vao);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(m_vertex_count));
    glBindVertexArray(0);
}

} // namespace pf
