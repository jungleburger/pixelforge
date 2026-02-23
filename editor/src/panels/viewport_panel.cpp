#include "viewport_panel.hpp"
#include <imgui.h>

namespace pf::editor {

void ViewportPanel::draw(EditorContext& /*ctx*/) {
    ImGui::Begin("Viewport");
    ImGui::Text("Zoom: %.2f  Pan: (%.0f, %.0f)", m_zoom, m_pan_x, m_pan_y);
    if (ImGui::IsWindowHovered()) {
        m_zoom += ImGui::GetIO().MouseWheel * 0.1f;
        m_zoom  = std::max(0.1f, m_zoom);
    }
    ImGui::End();
}

} // namespace pf::editor
