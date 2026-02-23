#include "performance_panel.hpp"
#include <imgui.h>
#include <format>
#include <numeric>

namespace pf::editor {

void PerformancePanel::record_frame(float dt) {
    m_frame_ms = dt * 1000.f;
    m_fps      = (dt > 0.f) ? 1.f / dt : 0.f;

    m_fps_history[m_history_offset] = m_fps;
    m_history_offset = (m_history_offset + 1) % HISTORY;
}

void PerformancePanel::draw(EditorContext& ctx) {
    ImGui::Begin("Performance");

    // ── Frame timing ──────────────────────────────────────────────────────────
    ImGui::SeparatorText("Frame");
    ImGui::Text("FPS:      %.1f", m_fps);
    ImGui::Text("Frame:    %.2f ms", m_frame_ms);

    // Rolling average
    const float avg = std::accumulate(m_fps_history.begin(),
                                       m_fps_history.end(), 0.f) /
                      static_cast<float>(HISTORY);
    ImGui::Text("Avg FPS:  %.1f  (last %d frames)", avg, HISTORY);

    // FPS sparkline
    const auto fps_overlay = std::format("{:.0f} fps", m_fps);
    ImGui::PlotLines("##fps_plot",
                     m_fps_history.data(),
                     HISTORY,
                     m_history_offset,
                     fps_overlay.c_str(),
                     0.f, 200.f,
                     {ImGui::GetContentRegionAvail().x, 50.f});

    // ── Simulation stats ──────────────────────────────────────────────────────
    ImGui::SeparatorText("Simulation");

    // Refresh pixel counts every 10 frames to avoid per-frame allocation overhead
    ++m_count_tick;
    if (m_count_tick >= 10) {
        m_count_tick = 0;
        if (ctx.world) {
            m_settled_count = ctx.world->lattice().size();
            m_dynamic_count = ctx.world->collect_all_dynamic().size();
        }
    }

    ImGui::Text("Settled px:  %zu", m_settled_count);
    ImGui::Text("Dynamic px:  %zu", m_dynamic_count);

    if (ctx.world) {
        ImGui::Text("Chunks:      %zu", ctx.world->chunk_count());
    }

    // ── Ambient conditions ────────────────────────────────────────────────────
    if (ctx.reactions) {
        ImGui::SeparatorText("Thermal");
        ImGui::Text("Ambient temp:       %.0f \u00b0C",
                    ctx.reactions->ambient_temperature);
        ImGui::Text("Conduction scale:   %.2f",
                    ctx.reactions->conduction_scale);
    }

    ImGui::End();
}

} // namespace pf::editor
