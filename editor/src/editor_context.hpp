#pragma once
#include <pixelforge/world/world.hpp>
#include <pixelforge/element/element_registry.hpp>

namespace pf::editor {

// Lightweight context passed to all panels/tools.
struct EditorContext {
    World*           world{nullptr};
    ElementRegistry* registry{nullptr};
    int              selected_element{0};
    int              brush_size{1};
    bool             show_grid{false};
};

} // namespace pf::editor
