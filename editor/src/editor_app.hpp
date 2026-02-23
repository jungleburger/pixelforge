#pragma once
#include <pixelforge/world/world.hpp>
#include <pixelforge/element/element_registry.hpp>
#include <pixelforge/physics/particle_system.hpp>
#include <pixelforge/reaction/reaction_system.hpp>
#include <pixelforge/procgen/biome.hpp>
#include <pixelforge/renderer/gl_renderer.hpp>
#include <pixelforge/save/world_serialiser.hpp>
#include "editor_context.hpp"
#include "panels/viewport_panel.hpp"
#include "panels/inspector_panel.hpp"
#include "panels/element_editor_panel.hpp"
#include "panels/element_palette_panel.hpp"
#include "panels/console_panel.hpp"
#include "panels/performance_panel.hpp"
#include "panels/hierarchy_panel.hpp"
#include "panels/worldgen_panel.hpp"
#include "panels/asset_browser_panel.hpp"
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
    std::unique_ptr<BiomeRegistry>   m_biomes;
    std::unique_ptr<World>           m_world;
    std::unique_ptr<ParticleSystem>  m_particles;
    std::unique_ptr<ReactionSystem>  m_reactions;
    std::unique_ptr<GlRenderer>      m_renderer;

    EditorContext     m_ctx;

    // Panels
    ViewportPanel       m_viewport_panel;
    InspectorPanel      m_inspector_panel;
    ElementEditorPanel  m_element_editor_panel;
    ElementPalettePanel m_palette_panel;
    ConsolePanel        m_console_panel;
    PerformancePanel    m_perf_panel;
    HierarchyPanel      m_hierarchy_panel;
    WorldgenPanel       m_worldgen_panel;
    AssetBrowserPanel   m_asset_browser_panel;

    bool  m_running{false};
    float m_last_time{0.f};
};

} // namespace pf::editor
