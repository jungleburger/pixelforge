#include "worldgen_panel.hpp"
#include <imgui.h>

namespace pf::editor {
void WorldgenPanel::draw(EditorContext& /*ctx*/) {
    ImGui::Begin("World Gen");
    ImGui::InputInt("Seed",   &m_seed);
    ImGui::InputInt("Width",  &m_width);
    ImGui::InputInt("Height", &m_height);
    if (ImGui::Button("Generate")) {
        // Integration point: trigger WorldGenerator
    }
    ImGui::End();
}
} // namespace pf::editor
