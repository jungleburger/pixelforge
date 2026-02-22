#pragma once
#include <pixelforge/world/world.hpp>
#include <pixelforge/element/element_registry.hpp>
#include <pixelforge/physics/particle_system.hpp>
#include <pixelforge/renderer/gl_renderer.hpp>
#include <memory>
#include <string>

// Forward declarations — ImGui is only used in editor/
struct SDL_Window;

namespace pf::editor {

class EditorApp {
public:
    EditorApp();
    ~EditorApp();

    [[nodiscard]] bool init(int window_w = 1600, int window_h = 900);
    void run();
    void shutdown();

private:
    void process_events();
    void update(float dt);
    void render();

    SDL_Window*                     m_window{nullptr};
    void*                           m_gl_ctx{nullptr};

    std::unique_ptr<ElementRegistry> m_registry;
    std::unique_ptr<World>           m_world;
    std::unique_ptr<ParticleSystem>  m_particles;
    std::unique_ptr<GlRenderer>      m_renderer;

    bool  m_running{false};
    float m_last_time{0.f};
};

} // namespace pf::editor
