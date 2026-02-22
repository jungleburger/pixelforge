#include "performance_panel.hpp"
#include <imgui.h>

namespace pf::editor {

void PerformancePanel::record_frame(float dt) {
    m_frame_ms = dt * 1000.f;
    m_fps      = (dt > 0.f) ? 1.f / dt : 0.f;
}

void PerformancePanel::draw(EditorContext& /*ctx*/) {
    ImGui::Begin("Performance");
    ImGui::Text("FPS:     %.1f", m_fps);
    ImGui::Text("Frame:   %.2f ms", m_frame_ms);
    ImGui::End();
}

} // namespace pf::editor
