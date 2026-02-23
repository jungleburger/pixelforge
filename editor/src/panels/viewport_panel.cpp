#include "viewport_panel.hpp"
#include <imgui.h>
#include <format>

namespace pf::editor {

void ViewportPanel::draw(EditorContext& ctx) {
    ImGui::Begin("Viewport");

    // ── Toolbar ───────────────────────────────────────────────────────────────
    ImGui::Text("Zoom: %.2f  Pan: (%.0f, %.0f)", m_zoom, m_pan_x, m_pan_y);
    ImGui::SameLine(0, 20);
    ImGui::Checkbox("Grid", &ctx.show_grid);
    ImGui::SameLine();
    ImGui::Checkbox("Temp Overlay", &ctx.show_temp_overlay);
    ImGui::SameLine();
    ImGui::Checkbox("Heat Brush", &ctx.heat_brush_active);
    if (ctx.heat_brush_active) {
        ImGui::SameLine(0, 8);
        ImGui::SetNextItemWidth(100.f);
        ImGui::SliderFloat("##heat", &ctx.heat_brush_amount, -500.f, 2000.f, "%.0f°C");
    }

    ImGui::Separator();

    // ── Status overlays ───────────────────────────────────────────────────────
    if (ctx.show_temp_overlay) {
        ImGui::TextColored({1.f, 0.6f, 0.1f, 1.f},
                           "  \u2714 Temperature overlay ON"
                           "  (cold = blue  \u2192  hot = red)");
    }
    if (ctx.heat_brush_active) {
        const char* sign = ctx.heat_brush_amount >= 0.f ? "+" : "";
        ImGui::TextColored({1.f, 0.3f, 0.3f, 1.f},
                           "  \ud83d\udd25 Heat brush active:  %s%.0f \u00b0C per click",
                           sign, ctx.heat_brush_amount);
    }

    // ── Zoom / scroll ─────────────────────────────────────────────────────────
    if (ImGui::IsWindowHovered()) {
        m_zoom += ImGui::GetIO().MouseWheel * 0.1f;
        m_zoom  = std::max(0.1f, m_zoom);
    }

    // Placeholder for the actual rendered pixel texture
    // (GlRenderer output is composited behind ImGui in the main render loop)
    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x > 0 && avail.y > 0) {
        ImGui::InvisibleButton("##viewport_canvas", avail);
    }

    ImGui::End();
}

} // namespace pf::editor
