#include "hierarchy_panel.hpp"
#include <imgui.h>

namespace pf::editor {
void HierarchyPanel::draw(EditorContext& ctx) {
    ImGui::Begin("Hierarchy");
    if (ctx.world) {
        ImGui::Text("Chunks: %zu", ctx.world->chunk_count());
        ImGui::Text("Lattice cells: %zu", ctx.world->lattice().size());
    }
    ImGui::End();
}
} // namespace pf::editor
