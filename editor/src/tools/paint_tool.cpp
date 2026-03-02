#include "paint_tool.hpp"
#include <pixelforge/pixel/dynamic_pixel.hpp>
#include <cmath>

namespace pf::editor {

void PaintTool::on_mouse_down(float wx, float wy, EditorContext& ctx) {
    on_mouse_drag(wx, wy, ctx);
}

void PaintTool::on_mouse_drag(float wx, float wy, EditorContext& ctx) {
    if (!ctx.world || !ctx.registry) return;

    const auto elem = static_cast<pf::ElementID>(ctx.selected_element);
    const pf::ElementDef* def = ctx.registry->get(elem);
    if (!def) return;

    const int w = ctx.world->config().width;
    const int h = ctx.world->config().height;
    const int r = ctx.brush_size - 1;

    // Integer cell of the mouse for the brush grid.
    const int cx = static_cast<int>(std::round(wx));
    const int cy = static_cast<int>(std::round(wy));

    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            const int px = cx + dx;
            const int py = cy + dy;
            if (px < 0 || px >= w || py < 0 || py >= h) continue;

            if (ctx.paint_as_settled) {
                // Place directly into the lattice (no physics).
                pf::SettledPixel sp;
                sp.element   = elem;
                sp.color     = def->color;
                sp.world_pos = {px, py};
                ctx.world->set_settled(px, py, sp);
            } else {
                // Skip if there is already a settled pixel here.
                if (ctx.world->has_settled(px, py)) continue;

                // Also skip if there is already a dynamic pixel falling
                // through this cell (avoid duplicate spawns from mouse drag).
                bool has_dynamic = false;
                for (const auto* dp : ctx.world->collect_all_dynamic()) {
                    if (!dp->awake) continue;
                    const int dpx = static_cast<int>(std::round(dp->pos.x));
                    const int dpy = static_cast<int>(std::round(dp->pos.y));
                    if (dpx == px && dpy == py) { has_dynamic = true; break; }
                }
                if (has_dynamic) continue;

                // Spawn at the exact sub-pixel mouse position (centre cell)
                // or grid-offset for brush cells around the centre.
                float spawn_x = wx + static_cast<float>(dx);
                float spawn_y = wy + static_cast<float>(dy);

                pf::DynamicPixel dp;
                dp.base.element = elem;
                dp.base.color   = def->color;
                dp.pos = {spawn_x, spawn_y};
                dp.vel = {0.f, 0.f};
                ctx.world->add_dynamic(dp);
            }
        }
    }
}

} // namespace pf::editor
