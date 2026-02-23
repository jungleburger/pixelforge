#include "inspector_panel.hpp"
#include <imgui.h>

namespace pf::editor {
void InspectorPanel::draw(EditorContext& ctx) {
    ImGui::Begin("Inspector");
    if (ctx.registry) {
        const auto* def = ctx.registry->get(
            static_cast<pf::ElementID>(ctx.selected_element));
        if (def) {
            ImGui::Text("Element: %s", def->name.c_str());
            ImGui::Text("Tag: %s",     def->tag.c_str());
            ImGui::Text("Density: %.2f", def->density);
        }
    }
    ImGui::End();
}
} // namespace pf::editor
