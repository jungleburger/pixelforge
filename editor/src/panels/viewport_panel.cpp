#include "viewport_panel.hpp"
#include <imgui.h>
#include <algorithm>

namespace pf::editor {

// ── Helpers ───────────────────────────────────────────────────────────────────

void ViewportPanel::world_coords(float mx, float my, int& wx, int& wy) const noexcept {
    wx = static_cast<int>((mx - m_canvas_x - m_pan_x) / m_zoom);
    wy = static_cast<int>((my - m_canvas_y - m_pan_y) / m_zoom);
}

void ViewportPanel::dispatch_tool(float mx, float my,
                                   bool pressed, bool held, bool released,
                                   EditorContext& ctx) {
    int wx{}, wy{};
    world_coords(mx, my, wx, wy);

    switch (ctx.active_tool) {
        case ActiveTool::Paint:
            if (pressed) m_paint.on_mouse_down(wx, wy, ctx);
            else if (held) m_paint.on_mouse_drag(wx, wy, ctx);
            break;

        case ActiveTool::Erase:
            if (pressed) m_erase.on_mouse_down(wx, wy, ctx);
            else if (held) m_erase.on_mouse_drag(wx, wy, ctx);
            break;

        case ActiveTool::Select:
            if (pressed)        m_select.on_mouse_down(wx, wy, ctx);
            else if (held)      m_select.on_mouse_drag(wx, wy, ctx);
            else if (released)  m_select.on_mouse_up(wx, wy, ctx);
            ctx.selection_box = m_select.selection();
            break;
    }
}

// ── Main draw ─────────────────────────────────────────────────────────────────

void ViewportPanel::draw(EditorContext& ctx) {
    ImGui::Begin("Viewport");
    ImGuiIO& io = ImGui::GetIO();

    // ── Keyboard shortcuts (tool switch) ─────────────────────────────────────
    if (!io.WantCaptureKeyboard) {
        if (ImGui::IsKeyPressed(ImGuiKey_P)) ctx.active_tool = ActiveTool::Paint;
        if (ImGui::IsKeyPressed(ImGuiKey_E)) ctx.active_tool = ActiveTool::Erase;
        if (ImGui::IsKeyPressed(ImGuiKey_B)) ctx.active_tool = ActiveTool::Select;
    }

    // ── Toolbar ───────────────────────────────────────────────────────────────
    auto tool_button = [&](const char* label, ActiveTool tool) {
        const bool active = (ctx.active_tool == tool);
        if (active) ImGui::PushStyleColor(ImGuiCol_Button,
                        ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if (ImGui::SmallButton(label)) ctx.active_tool = tool;
        if (active) ImGui::PopStyleColor();
    };
    tool_button("Paint [P]",  ActiveTool::Paint);
    ImGui::SameLine();
    tool_button("Erase [E]",  ActiveTool::Erase);
    ImGui::SameLine();
    tool_button("Select [B]", ActiveTool::Select);

    ImGui::SameLine(0, 16);
    ImGui::SetNextItemWidth(80.f);
    ImGui::SliderInt("Brush", &ctx.brush_size, 1, 16);

    ImGui::SameLine(0, 16);
    ImGui::Text("Zoom %.2f×", m_zoom);
    ImGui::SameLine(0, 10);
    ImGui::Checkbox("Grid",     &ctx.show_grid);
    ImGui::SameLine();
    ImGui::Checkbox("Temp",     &ctx.show_temp_overlay);
    ImGui::SameLine();
    ImGui::Checkbox("Heat",     &ctx.heat_brush_active);
    if (ctx.heat_brush_active) {
        ImGui::SameLine(0, 8);
        ImGui::SetNextItemWidth(110.f);
        ImGui::SliderFloat("##heat", &ctx.heat_brush_amount,
                           -500.f, 2000.f, "%+.0f\u00b0C");
    }

    ImGui::Separator();

    // ── Status line ───────────────────────────────────────────────────────────
    if (ctx.show_temp_overlay)
        ImGui::TextColored({1.f, 0.6f, 0.1f, 1.f},
                           "  \u2714 Temperature overlay  (cold=blue \u2192 hot=red)");
    if (ctx.heat_brush_active)
        ImGui::TextColored({1.f, 0.3f, 0.3f, 1.f},
                           "  [Heat brush] %+.0f\u00b0C/click",
                           ctx.heat_brush_amount);
    if (ctx.selection_box.has_value()) {
        const auto& b = *ctx.selection_box;
        ImGui::TextColored({0.4f, 1.f, 0.4f, 1.f},
                           "  \u25a1 Selection  (%d, %d)  %d \u00d7 %d",
                           b.x, b.y, b.w, b.h);
    }

    // ── Viewport canvas ───────────────────────────────────────────────────────
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x <= 0.f || avail.y <= 0.f) { ImGui::End(); return; }

    const ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    m_canvas_x = canvas_pos.x;
    m_canvas_y = canvas_pos.y;

    // Invisible button captures mouse events for the canvas area
    ImGui::InvisibleButton("##vp_canvas", avail,
        ImGuiButtonFlags_MouseButtonLeft  |
        ImGuiButtonFlags_MouseButtonRight |
        ImGuiButtonFlags_MouseButtonMiddle);

    const bool hovered  = ImGui::IsItemHovered();
    const bool pressed  = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    const bool held     = ImGui::IsMouseDown(ImGuiMouseButton_Left) && hovered;
    const bool released = ImGui::IsMouseReleased(ImGuiMouseButton_Left);

    // Scroll to zoom (keep the point under cursor stationary)
    if (hovered && io.MouseWheel != 0.f) {
        const ImVec2 mp  = io.MousePos;
        const float  pre = m_zoom;
        m_zoom = std::max(0.1f, m_zoom + io.MouseWheel * 0.15f * m_zoom);
        const float scale = m_zoom / pre;
        m_pan_x = mp.x - m_canvas_x - (mp.x - m_canvas_x - m_pan_x) * scale;
        m_pan_y = mp.y - m_canvas_y - (mp.y - m_canvas_y - m_pan_y) * scale;
    }

    // Middle-mouse drag to pan
    if (hovered && ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
        m_pan_x += io.MouseDelta.x;
        m_pan_y += io.MouseDelta.y;
    }

    // Right-click clears selection
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        ctx.selection_box.reset();

    // Heat brush on left-click (only on first press, not drag)
    if (pressed && ctx.heat_brush_active && ctx.reactions) {
        int wx{}, wy{};
        world_coords(io.MousePos.x, io.MousePos.y, wx, wy);
        ctx.reactions->apply_heat(wx, wy, ctx.heat_brush_amount);
    }

    // Tool dispatch (only when NOT using heat brush, or always let both fire)
    if (hovered && (pressed || held || released)) {
        dispatch_tool(io.MousePos.x, io.MousePos.y,
                      pressed, held && !pressed, released && !pressed && !held,
                      ctx);
    }

    // ── Selection overlay drawn on top ────────────────────────────────────────
    if (ctx.selection_box.has_value()) {
        const auto& sel = *ctx.selection_box;
        const ImVec2 a{
            m_canvas_x + m_pan_x + sel.x * m_zoom,
            m_canvas_y + m_pan_y + sel.y * m_zoom
        };
        const ImVec2 b{
            a.x + sel.w * m_zoom,
            a.y + sel.h * m_zoom
        };
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRect(a, b, IM_COL32(64, 220, 64, 220), 0.f, 0, 1.5f);
        // Dashed corner handles
        constexpr float H = 6.f;
        dl->AddLine({a.x, a.y}, {a.x + H, a.y},      IM_COL32(255,255,255,200), 2.f);
        dl->AddLine({a.x, a.y}, {a.x, a.y + H},      IM_COL32(255,255,255,200), 2.f);
        dl->AddLine({b.x, b.y}, {b.x - H, b.y},      IM_COL32(255,255,255,200), 2.f);
        dl->AddLine({b.x, b.y}, {b.x, b.y - H},      IM_COL32(255,255,255,200), 2.f);
    }

    ImGui::End();
}

} // namespace pf::editor

