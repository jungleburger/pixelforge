#include "erase_tool.hpp"

namespace pf::editor {

void EraseTool::on_mouse_down(int wx, int wy, EditorContext& ctx) {
    on_mouse_drag(wx, wy, ctx);
}

void EraseTool::on_mouse_drag(int wx, int wy, EditorContext& ctx) {
    if (!ctx.world) return;
    const int w = ctx.world->config().width;
    const int h = ctx.world->config().height;
    const int r = ctx.brush_size - 1;
    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            const int px = wx + dx;
            const int py = wy + dy;
            if (px < 0 || px >= w || py < 0 || py >= h) continue;
            ctx.world->remove_settled(px, py);
        }
    }
}

} // namespace pf::editor
