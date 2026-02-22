#include "select_tool.hpp"
#include <algorithm>

namespace pf::editor {

void SelectTool::on_mouse_down(int wx, int wy, EditorContext& /*ctx*/) {
    m_dragging = true;
    m_start    = {wx, wy};
    m_selection.reset();
}

void SelectTool::on_mouse_drag(int wx, int wy, EditorContext& /*ctx*/) {
    if (!m_dragging) return;
    pf::AABB box;
    box.x = std::min(m_start.x, wx);
    box.y = std::min(m_start.y, wy);
    box.w = std::abs(wx - m_start.x) + 1;
    box.h = std::abs(wy - m_start.y) + 1;
    m_selection = box;
}

void SelectTool::on_mouse_up(int wx, int wy, EditorContext& ctx) {
    on_mouse_drag(wx, wy, ctx);
    m_dragging = false;
}

} // namespace pf::editor
