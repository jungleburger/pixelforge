#include "console_panel.hpp"
#include <imgui.h>

namespace pf::editor {

void ConsolePanel::push(std::string msg) {
    m_lines.push_back(std::move(msg));
    if (m_lines.size() > MAX_LINES) m_lines.pop_front();
}

void ConsolePanel::draw(EditorContext& /*ctx*/) {
    ImGui::Begin("Console");
    for (const auto& line : m_lines) {
        ImGui::TextUnformatted(line.c_str());
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::End();
}

} // namespace pf::editor
