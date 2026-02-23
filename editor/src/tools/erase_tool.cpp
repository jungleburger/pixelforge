#include "erase_tool.hpp"

namespace pf::editor {

void EraseTool::on_mouse_down(int wx, int wy, EditorContext& ctx) {
    on_mouse_drag(wx, wy, ctx);
}

void EraseTool::on_mouse_drag(int wx, int wy, EditorContext& ctx) {
    if (!ctx.world) return;
    const int r = ctx.brush_size - 1;
    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            ctx.world->remove_settled(wx + dx, wy + dy);
        }
    }
}

} // namespace pf::editor
