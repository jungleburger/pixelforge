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
