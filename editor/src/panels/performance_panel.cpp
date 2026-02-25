#include "performance_panel.hpp"
#include <imgui.h>
#include <format>
#include <numeric>

namespace pf::editor {

void PerformancePanel::record_frame(float dt) {
    m_frame_ms = dt * 1000.f;
    m_fps      = (dt > 0.f) ? 1.f / dt : 0.f;

    // Track min/max (ignore the initial zero)
    if (m_fps > 0.f) {
        if (m_fps < m_fps_min) m_fps_min = m_fps;
        if (m_fps > m_fps_max) m_fps_max = m_fps;
    }

    m_fps_history[m_history_offset] = m_fps;
    m_history_offset = (m_history_offset + 1) % HISTORY;
}

void PerformancePanel::record_subsystem_times(const SubsystemTimes& t) {
    m_sys_times = t;
}

void PerformancePanel::draw(EditorContext& ctx) {
    ImGui::Begin("Performance");

    // â”€â”€ Frame timing â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    ImGui::SeparatorText("Frame");
    ImGui::Text("FPS:      %.1f", m_fps);
    ImGui::Text("Frame:    %.2f ms", m_frame_ms);

    // Rolling average
    const float avg = std::accumulate(m_fps_history.begin(),
                                       m_fps_history.end(), 0.f) /
                      static_cast<float>(HISTORY);
    ImGui::Text("Avg FPS:  %.1f  (last %d frames)", avg, HISTORY);

    const bool has_minmax = (m_fps_min < std::numeric_limits<float>::max());
    if (has_minmax) {
        ImGui::Text("Min FPS:  %.1f", m_fps_min);
        ImGui::Text("Max FPS:  %.1f", m_fps_max);
    }

    // FPS sparkline
    const auto fps_overlay = std::format("{:.0f} fps", m_fps);
    ImGui::PlotLines("##fps_plot",
                     m_fps_history.data(),
                     HISTORY,
                     m_history_offset,
                     fps_overlay.c_str(),
                     0.f, 200.f,
                     {ImGui::GetContentRegionAvail().x, 48.f});

    // Reset min/max button
    ImGui::SameLine();
    if (ImGui::SmallButton("Reset")) {
        m_fps_min = std::numeric_limits<float>::max();
        m_fps_max = 0.f;
    }

    // â”€â”€ Sub-system timings â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    ImGui::SeparatorText("Sub-system (ms)");

    const float bar_max = 16.7f;  // one frame @ 60 fps
    auto timing_bar = [&](const char* label, float ms) {
        ImGui::Text("%-18s %.2f ms", label, ms);
        ImGui::SameLine();
        const float frac = ms / bar_max;
        ImGui::ProgressBar(frac < 1.f ? frac : 1.f,
                           {80.f, 0.f}, "");
    };

    timing_bar("Physics:",         m_sys_times.physics_ms);
    timing_bar("Reactions:",        m_sys_times.reactions_ms);
    timing_bar("Renderer upload:",  m_sys_times.renderer_upload_ms);
    timing_bar("ImGui:",            m_sys_times.imgui_ms);

    const float total_accounted = m_sys_times.physics_ms
                                + m_sys_times.reactions_ms
                                + m_sys_times.renderer_upload_ms
                                + m_sys_times.imgui_ms;
    timing_bar("Other:",  m_frame_ms - total_accounted > 0.f
                          ? m_frame_ms - total_accounted : 0.f);

    // â”€â”€ Simulation stats â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    ImGui::SeparatorText("Simulation");

    // Refresh every 10 frames to avoid per-frame allocation overhead
    ++m_count_tick;
    if (m_count_tick >= 10) {
        m_count_tick = 0;
        if (ctx.world) {
            m_settled_count = ctx.world->lattice().size();
            m_dynamic_count = ctx.world->collect_all_dynamic().size();
            m_bond_count    = ctx.world->bonds().size();
            m_awake_chunks  = ctx.world->awake_chunk_count();
        }
    }

    // Format large numbers with thousands separator
    auto fmt_num = [](size_t n) -> std::string {
        std::string s = std::to_string(n);
        for (int i = static_cast<int>(s.size()) - 3; i > 0; i -= 3)
            s.insert(static_cast<size_t>(i), ",");
        return s;
    };

    ImGui::Text("Settled px:  %s", fmt_num(m_settled_count).c_str());
    ImGui::Text("Dynamic px:  %s", fmt_num(m_dynamic_count).c_str());
    ImGui::Text("Bonds:       %s", fmt_num(m_bond_count).c_str());

    if (ctx.world) {
        const size_t total_chunks = ctx.world->chunk_count();
        ImGui::Text("Chunks:      %zu awake / %zu total",
                    m_awake_chunks, total_chunks);
    }

    // Memory estimate: rough bytes-per-pixel
    // SettledPixel ~32B, DynamicPixel ~80B, Bond ~32B
    constexpr size_t SETTLED_BYTES = 32;
    constexpr size_t DYNAMIC_BYTES = 80;
    constexpr size_t BOND_BYTES    = 32;
    const size_t mem_kb = (m_settled_count * SETTLED_BYTES
                         + m_dynamic_count * DYNAMIC_BYTES
                         + m_bond_count    * BOND_BYTES) / 1024;
    ImGui::TextDisabled("Est. sim memory: %zu KB", mem_kb);

    // â”€â”€ Ambient conditions â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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
