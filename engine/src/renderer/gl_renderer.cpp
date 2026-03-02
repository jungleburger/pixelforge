#include <pixelforge/renderer/gl_renderer.hpp>
#include <pixelforge/core/logger.hpp>
#include <glad/glad.h>
#include <SDL3/SDL.h>

namespace pf {

GlRenderer::GlRenderer()
    : m_settled(std::make_unique<SettledLayer>())
    , m_dynamic(std::make_unique<DynamicLayer>())
{}

GlRenderer::~GlRenderer() { shutdown(); }

bool GlRenderer::init(void* sdl_window, int viewport_w, int viewport_h) {
    m_vp_w = viewport_w;
    m_vp_h = viewport_h;

    // Load GL via glad
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress))) {
        PF_LOG_ERROR("GlRenderer: failed to load OpenGL via glad");
        return false;
    }

    PF_LOG_INFO("GlRenderer: OpenGL {}", (const char*)glGetString(GL_VERSION));

    glViewport(0, 0, viewport_w, viewport_h);
    glClearColor(0.05f, 0.05f, 0.05f, 1.f);

    m_dynamic->init();
    return true;
}

void GlRenderer::shutdown() {
    m_settled->shutdown();
    m_dynamic->shutdown();
    if (m_fbo_tex) { glDeleteTextures(1, &m_fbo_tex); m_fbo_tex = 0; }
    if (m_fbo)     { glDeleteFramebuffers(1, &m_fbo); m_fbo = 0; }
}

bool GlRenderer::init_fbo(int world_w, int world_h) {
    // Clean up previous FBO if any
    if (m_fbo_tex) { glDeleteTextures(1, &m_fbo_tex); m_fbo_tex = 0; }
    if (m_fbo)     { glDeleteFramebuffers(1, &m_fbo); m_fbo = 0; }

    m_fbo_w = world_w;
    m_fbo_h = world_h;

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glGenTextures(1, &m_fbo_tex);
    glBindTexture(GL_TEXTURE_2D, m_fbo_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, world_w, world_h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, m_fbo_tex, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        PF_LOG_ERROR("GlRenderer: FBO incomplete");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    PF_LOG_INFO("GlRenderer: FBO created {}x{}", world_w, world_h);
    return true;
}

void GlRenderer::render_world() {
    if (!m_fbo) return;

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_fbo_w, m_fbo_h);
    glClearColor(0.05f, 0.05f, 0.05f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Orthographic projection: (0,0) top-left, (w,h) bottom-right
    const float l = 0.f, r = static_cast<float>(m_fbo_w);
    const float bt = static_cast<float>(m_fbo_h), t = 0.f;
    const float n = -1.f, f = 1.f;
    float mvp[16]{};
    mvp[0]  = 2.f / (r - l);
    mvp[5]  = 2.f / (t - bt);
    mvp[10] = -2.f / (f - n);
    mvp[12] = -(r + l) / (r - l);
    mvp[13] = -(t + bt) / (t - bt);
    mvp[14] = -(f + n) / (f - n);
    mvp[15] = 1.f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_settled->draw(mvp);
    m_dynamic->draw(mvp);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_vp_w, m_vp_h);
}

void GlRenderer::begin_frame() {
    glClear(GL_COLOR_BUFFER_BIT);
}

void GlRenderer::end_frame() {
    // Swap is handled by SDL in the application layer.
}

void GlRenderer::resize(int w, int h) {
    m_vp_w = w;
    m_vp_h = h;
    glViewport(0, 0, w, h);
}

} // namespace pf
