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

    [[nodiscard]] SettledLayer&  settled_layer()  { return *m_settled; }
    [[nodiscard]] DynamicLayer&  dynamic_layer()  { return *m_dynamic; }

private:
    int m_vp_w{0}, m_vp_h{0};
    std::unique_ptr<SettledLayer> m_settled;
    std::unique_ptr<DynamicLayer> m_dynamic;
};

} // namespace pf
