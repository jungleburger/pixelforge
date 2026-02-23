#pragma once
#include <pixelforge/world/world.hpp>
#include <pixelforge/element/element_registry.hpp>
#include <pixelforge/reaction/reaction_system.hpp>

namespace pf::editor {

// Lightweight context passed to all panels/tools.
struct EditorContext {
    World*           world{nullptr};
    ElementRegistry* registry{nullptr};
    ReactionSystem*  reactions{nullptr};
    int              selected_element{0};
    int              brush_size{1};
    bool             show_grid{false};
    // Heat-brush tool (Phase 4)
    bool             heat_brush_active{false};
    float            heat_brush_amount{200.f};  ///< Degrees per click
    bool             show_temp_overlay{false};  ///< Visualise temperature in viewport
};

} // namespace pf::editor
