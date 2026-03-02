#pragma once
#include <pixelforge/renderer/renderer.hpp>
#include <pixelforge/renderer/settled_layer.hpp>
#include <pixelforge/renderer/dynamic_layer.hpp>
#include <cstdint>
#include <memory>

namespace pf {

class GlRenderer : public IRenderer {
public:
    GlRenderer();
    ~GlRenderer() override;

    bool init(void* sdl_window, int viewport_w, int viewport_h) override;
    void shutdown() override;

    void begin_frame() override;
    void end_frame()   override;

    void resize(int w, int h) override;

    /// (Re-)create the off-screen FBO sized to the world.
    bool init_fbo(int world_w, int world_h);

    /// Render settled + dynamic layers into the internal FBO.
    void render_world();

    /// GL texture ID of the composited world image (for ImGui display).
    [[nodiscard]] uint32_t world_texture() const { return m_fbo_tex; }

    [[nodiscard]] SettledLayer&  settled_layer()  { return *m_settled; }
    [[nodiscard]] DynamicLayer&  dynamic_layer()  { return *m_dynamic; }

private:
    int m_vp_w{0}, m_vp_h{0};
    std::unique_ptr<SettledLayer> m_settled;
    std::unique_ptr<DynamicLayer> m_dynamic;

    // Off-screen FBO for world compositing
    uint32_t m_fbo{0};
    uint32_t m_fbo_tex{0};
    int m_fbo_w{0}, m_fbo_h{0};
};

} // namespace pf
