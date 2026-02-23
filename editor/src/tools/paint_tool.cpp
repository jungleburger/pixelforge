#include "paint_tool.hpp"

namespace pf::editor {

void PaintTool::on_mouse_down(int wx, int wy, EditorContext& ctx) {
    on_mouse_drag(wx, wy, ctx);
}

void PaintTool::on_mouse_drag(int wx, int wy, EditorContext& ctx) {
    if (!ctx.world || !ctx.registry) return;

    const auto elem = static_cast<pf::ElementID>(ctx.selected_element);
    const pf::ElementDef* def = ctx.registry->get(elem);
    if (!def) return;

    const int r = ctx.brush_size - 1;
    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            pf::SettledPixel sp;
            sp.element   = elem;
            sp.color     = def->color;
            sp.world_pos = {wx + dx, wy + dy};
            ctx.world->set_settled(wx + dx, wy + dy, sp);
        }
    }
}

} // namespace pf::editor
