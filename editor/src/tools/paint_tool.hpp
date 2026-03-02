#pragma once
#include "../editor_context.hpp"

namespace pf::editor {
class PaintTool {
public:
    void on_mouse_down(float wx, float wy, EditorContext& ctx);
    void on_mouse_drag(float wx, float wy, EditorContext& ctx);
};
} // namespace pf::editor
