#pragma once
#include "../editor_context.hpp"

namespace pf::editor {
class PaintTool {
public:
    void on_mouse_down(int wx, int wy, EditorContext& ctx);
    void on_mouse_drag(int wx, int wy, EditorContext& ctx);
};
} // namespace pf::editor
