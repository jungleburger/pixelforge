#pragma once
#include <pixelforge/world/world.hpp>
#include <pixelforge/element/element_registry.hpp>
#include <pixelforge/reaction/reaction_system.hpp>
#include <pixelforge/procgen/biome.hpp>
#include <pixelforge/physics/particle_system.hpp>
#include <pixelforge/core/types.hpp>
#include <optional>
#include <string>

namespace pf::editor {

/// Which interaction tool is currently active in the Viewport.
enum class ActiveTool { Paint, Erase, Select };

// Lightweight context passed to all panels/tools.
struct EditorContext {
    World*           world{nullptr};
    ElementRegistry* registry{nullptr};
    ReactionSystem*  reactions{nullptr};
    BiomeRegistry*   biomes{nullptr};       ///< Phase 5: needed by WorldgenPanel
    ParticleSystem*  particles{nullptr};    ///< Phase 5: needed by PerformancePanel

    int              selected_element{0};
    int              brush_size{1};
    bool             show_grid{false};

    // Heat-brush tool (Phase 4)
    bool             heat_brush_active{false};
    float            heat_brush_amount{200.f};  ///< Degrees per click
    bool             show_temp_overlay{false};  ///< Visualise temperature in viewport

    // Phase 5: active tool + selection
    ActiveTool                active_tool{ActiveTool::Paint};
    std::optional<pf::AABB>   selection_box;   ///< Set by SelectTool, cleared on right-click

    // Phase 5: save / load path (editable in File menu)
    std::string               save_path{"world.pfw"};
};

} // namespace pf::editor
